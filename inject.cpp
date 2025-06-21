#include "inject.h"
#include "imgui.h"

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <algorithm>
#include <string>
#include <set>
#include <vector>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>

struct process
{
	std::string name;
	int32_t pid;
	FILETIME creationTime;

	bool operator<(const process& other) const 
	{
		return CompareFileTime(&creationTime, &other.creationTime) > 0;
	}
};

std::vector<process> processes;
int32_t selected_process = -1;

std::vector<std::string> exclude_processes =
{
	"browser.exe", "Discord.exe", "svchost.exe", "Telegram.exe",
	"steamwebhelper.exe", "notepad++.exe", "vctip.exe", "vcpkgsrv.exe",
	"vshost.exe", "devenv.exe", "ServiceHub.", "RuntimeBroker.exe",
	"msvsmon.exe", "ShellExperienceHost.exe", "PerfWatson.exe",
	"SearchApp.exe", "RtkAudUService64.exe", "steam.exe", "MSBuild.exe", 
	"msedgewebview2.exe", "Lightshot.exe", "wallpaper32.exe", "TextInputHost.exe",
	"StartMenuExperienceHost.exe", "HudSightApp", "explorer.exe", "sihost.exe", "nvcontainer.exe",
	"conhost.exe", "VsDebugConsole.exe", "taskhostw.exe", "ApplicationFrameHost.exe", 
	"CompPkgSrv.exe", "PerfWatson2.exe", "PhoneExperienceHost.exe", "backgroundTaskHost.exe",
	"UserOOBEBroker.exe", "SearchProtocolHost.exe", "crashpad_handler.exe"
};

static std::string get_process_path(DWORD pid)
{
	char exePath[MAX_PATH] = { 0 };
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (!hProc) 
	{
		return "";
	}

	if (!GetModuleFileNameExA(hProc, NULL, exePath, MAX_PATH)) {
		CloseHandle(hProc);
		return "";
	}

	CloseHandle(hProc);

	std::string path = exePath;
	size_t lastSlash = path.find_last_of("\\/");
	return (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";
}

static bool show_dll_modal = false;
static std::vector<std::string> dll_files;
static std::string selected_dll;

std::string get_current_directory()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string path = buffer;
	return path.substr(0, path.find_last_of("\\/") + 1);
}

void refresh_dll_list()
{
	dll_files.clear();
	std::string folder = get_current_directory();
	for (const auto& entry : std::filesystem::directory_iterator(folder))
	{
		if (entry.path().extension() == ".dll")
			dll_files.push_back(entry.path().filename().string());
	}
}

bool load_dll_data(const std::string& path, std::vector<char>& outData)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) return false;
	outData.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	return true;
}


static bool inject_dll(DWORD pid, const char* dllName, void* dllData, size_t dllSize)
{
	std::string processDir = get_process_path(pid);
	if (processDir.empty()) return false;

	std::string fullDllPath = processDir + dllName;

	HANDLE hFile = CreateFileA(fullDllPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return false;
	}

	DWORD written;
	WriteFile(hFile, dllData, static_cast<DWORD>(dllSize), &written, NULL);
	CloseHandle(hFile);

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProc) 
	{
		DeleteFileA(fullDllPath.c_str());
		return false;
	}

	LPVOID pRemoteMem = VirtualAllocEx(hProc, NULL, fullDllPath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pRemoteMem) 
	{
		DeleteFileA(fullDllPath.c_str());
		CloseHandle(hProc);
		return false;
	}

	WriteProcessMemory(hProc, pRemoteMem, fullDllPath.c_str(), fullDllPath.size() + 1, NULL);

	HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pRemoteMem, 0, NULL);
	if (!hThread) 
	{
		DeleteFileA(fullDllPath.c_str());
		VirtualFreeEx(hProc, pRemoteMem, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return false;
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	CloseHandle(hProc);
	DeleteFileA(fullDllPath.c_str());

	return true;
}

static bool is_process_x64_bit(DWORD processID)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
	if (!hProcess) return false;

	BOOL isWow64 = FALSE;
	if (IsWow64Process(hProcess, &isWow64)) 
	{
		CloseHandle(hProcess);
		return !isWow64;
	}

	CloseHandle(hProcess);
	return false;
}

static bool GetProcessCreationTime(DWORD processID, FILETIME& creationTime)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
	if (!hProcess) return false;

	FILETIME exitTime, kernelTime, userTime;
	bool success = GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime);
	CloseHandle(hProcess);
	return success;
}

static void update_process_list()
{
	static std::chrono::steady_clock::time_point last_update_time;

	auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time).count() < 1)
		return;

	last_update_time = now;

	std::vector<process> new_processes;

	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return;

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			auto it = std::find_if(exclude_processes.begin(), exclude_processes.end(), [&pe32](const std::string& exclude)
				{
					return std::string(pe32.szExeFile).contains(exclude);
				});

			if (it == exclude_processes.end() && GetCurrentProcessId() != pe32.th32ProcessID && is_process_x64_bit(pe32.th32ProcessID))
			{
				FILETIME creationTime;
				if (GetProcessCreationTime(pe32.th32ProcessID, creationTime))
				{
					new_processes.emplace_back(pe32.szExeFile, pe32.th32ProcessID, creationTime);
				}
			}

		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);

	std::sort(new_processes.begin(), new_processes.end());

	processes = std::move(new_processes);
}

static void render_process_list()
{
	update_process_list();
	
	if (ImGui::BeginTable("##process_table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0.0, ImGui::GetContentRegionAvail().y - 33.0f)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_NoHeaderWidth);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHeaderWidth);
		ImGui::TableHeadersRow();

		for (const auto& proc : processes)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%i", proc.pid);

			ImGui::TableSetColumnIndex(1);
			bool is_selected = selected_process == proc.pid;
			if (ImGui::Selectable(std::format("{}##{}", proc.name, proc.pid).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns))
			{
				selected_process = proc.pid;
			}
		}

		ImGui::EndTable();
	}
}


void injector::render()
{
	const ImVec2 basesize(1.0f, 1.0f);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9.0, 0.0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0, 4.0));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(15.0, 4.0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(255, 25, 255, 0));

	ImGui::BeginChild("##4", ImVec2(0, 0) * basesize, ImGuiChildFlags_Border);

	{
		static float select_a_process_x = ImGui::CalcTextSize("Select a process... ").x;

		ImVec2 size = ImVec2(120.0f * 2.0f, 52.0f / 2.5f) * basesize;
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - select_a_process_x) * 0.5f);
		{
			ImGui::BeginChild("##2", size);
			ImGui::Text("Select a process...");
			ImGui::EndChild();
		}

		ImGui::Separator();
		ImGui::Spacing();

		render_process_list();
	}

	ImGui::Spacing();

	if (ImGui::Button("Attach", ImVec2(-1.0, 25.0f)))
	{
		if (!show_dll_modal)
		{
			refresh_dll_list();
			show_dll_modal = true;
			ImGui::OpenPopup("Select DLL");
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
	}

	if (ImGui::BeginPopupModal("Select DLL", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		for (const auto& file : dll_files)
		{
			if (ImGui::Selectable(file.c_str()))
			{
				selected_dll = file;
				show_dll_modal = false;
				ImGui::CloseCurrentPopup();

				std::string fullPath = get_current_directory() + selected_dll;
				std::vector<char> dllData;
				if (load_dll_data(fullPath, dllData))
				{
					inject_dll(selected_process, selected_dll.c_str(), dllData.data(), dllData.size());
				}
			}
		}

		if (ImGui::Button("Close"))
		{
			show_dll_modal = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::EndChild();

	ImGui::PopStyleColor(1);
	ImGui::PopStyleVar(3);
}
