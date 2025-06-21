#include "directx.h"

#include "imgui.h"

#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "misc/freetype/imgui_freetype.h"

#include "font/quicksand_semi_bold.h"

#include "inject.h"
 
#include <iostream>
#include <Windows.h>

directx dx;
window* main_window = nullptr;

ImFont* window_font, *window_font_small;

class window
{
public:
	window(const std::wstring& title, int width, int height)
		: _title(title), _width(width), _height(height), _should_resize(false)
	{
		_wnd_class = { sizeof(_wnd_class), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, _title.c_str(), nullptr };
		//m_wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
		  
		::RegisterClassExW(&_wnd_class);
		_window_handle = ::CreateWindowW(_wnd_class.lpszClassName, _title.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, _width, _height, nullptr, nullptr, _wnd_class.hInstance, nullptr);
		::ShowWindow(_window_handle, SW_SHOWDEFAULT);
		::UpdateWindow(_window_handle);

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	}

	~window()
	{
		::DestroyWindow(_window_handle);
		::UnregisterClassW(_wnd_class.lpszClassName, _wnd_class.hInstance);
	}

	void load_font()
	{
		ImGuiIO& io = ImGui::GetIO();

		ImFontConfig config;
		config.SizePixels = 16.0f;
		config.OversampleH = 4;
		config.OversampleV = 4;
		config.PixelSnapH = true;

		config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_MonoHinting;

		window_font = io.Fonts->AddFontFromMemoryCompressedTTF(g_Quicksand_Semibold_compressed_data, g_Quicksand_Semibold_compressed_size, 18.0f, &config, io.Fonts->GetGlyphRangesJapanese());
		window_font_small = io.Fonts->AddFontFromMemoryCompressedTTF(g_Quicksand_Semibold_compressed_data, g_Quicksand_Semibold_compressed_size, 15.0f, &config, io.Fonts->GetGlyphRangesJapanese());
	}

	void set_design()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = ImGui::GetStyle().Colors;

		style.WindowTitleAlign = ImVec2(0.5, 0.5);
		style.WindowBorderSize = 0.0f;
		style.FrameBorderSize = 0.0f;
		style.FrameRounding = 12.0f;
		style.GrabMinSize = 16.5f;
		style.GrabRounding = 12.0f;
		style.ChildBorderSize = 1.0f;
		style.ChildRounding = 12.0f;
		style.WindowPadding = ImVec2(14.0f, 10.0f);
		style.WindowRounding = 2.5f;
		style.PopupRounding = 12.0f;

		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

		//colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.13f, 1.00f);
		//colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.07f, 0.09f, 1.00f); 
		colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.07f, 0.10f, 0.77f);

		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.12f, 0.14f, 0.19f, 0.75f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.42f, 0.46f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.54f, 0.55f, 0.60f, 0.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.54f, 0.55f, 0.60f, 0.62f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.54f, 0.55f, 0.60f, 0.73f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

		colors[ImGuiCol_Separator] = ImVec4(0.04f, 0.31f, 0.26f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.05f, 0.35f, 0.55f, 0.89f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.05f, 0.35f, 0.55f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.02f, 0.42f, 0.38f, 0.79f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.02f, 0.32f, 0.31f, 0.85f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.02f, 0.32f, 0.31f, 0.91f);
		colors[ImGuiCol_Button] = ImVec4(0.02f, 0.42f, 0.38f, 0.87f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.02f, 0.42f, 0.38f, 0.93f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.03f, 0.41f, 0.67f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.02f, 0.23f, 0.26f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.02f, 0.29f, 0.31f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.02f, 0.30f, 0.31f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.06f, 0.22f, 0.28f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.09f, 0.25f, 0.31f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.08f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.15f, 0.18f, 0.50f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.50f, 0.65f, 0.68f, 0.53f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.02f, 0.42f, 0.38f, 0.87f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.13f, 0.64f, 0.36f, 0.70f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.13f, 0.44f, 0.67f, 0.84f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.29f, 0.30f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.02f, 0.29f, 0.30f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.15f, 0.18f, 0.75f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.15f, 0.18f, 0.75f);
	}

	void run()
	{
		::ShowWindow(_window_handle, SW_SHOWDEFAULT);
		::UpdateWindow(_window_handle);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		ImGui::StyleColorsDark();

		load_font();
		set_design(); 

		ImGui_ImplWin32_Init(_window_handle);
		ImGui_ImplDX11_Init(dx.get_device(), dx.get_device_context());

		bool done = false;

		while (!done)
		{
			MSG msg;
			while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				if (msg.message == WM_QUIT)
					done = true;
			}

			if (done)
				break;

			if (_should_resize)
			{
				dx.destroy_render_target();
				dx.get_swap_chain()->ResizeBuffers(0, _width, _height, DXGI_FORMAT_UNKNOWN, 0);
				dx.create_render_target();

				_should_resize = false;
			}

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			ImGuiIO& io = ImGui::GetIO();
			ImVec2 size = io.DisplaySize;
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
			ImGui::SetNextWindowSize(size);

			ImGui::Begin("##window_app", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

			injector::render();

			ImGui::End();
			ImGui::Render();

			ID3D11RenderTargetView* renderTargets[1];
			renderTargets[0] = dx.get_render_target();

			const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			dx.get_device_context()->OMSetRenderTargets(1, renderTargets, nullptr);
			dx.get_device_context()->ClearRenderTargetView(dx.get_render_target(), clearColor);

			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			dx.get_swap_chain()->Present(1, 0);
		}

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	inline HWND get_window_handle() const noexcept
	{
		return _window_handle;
	}

	void resize(int width, int height)
	{
		if (_width == width && height == _height)
			return;

		_should_resize = true;
		_width = width;
		_height = height;
	}

private:
	WNDCLASSEXW _wnd_class;

	HWND _window_handle;
	std::wstring _title;

	int _width;
	int _height;

	bool _should_resize;

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED)
				return 0;

			int width = (UINT)LOWORD(lParam);
			int height = (UINT)HIWORD(lParam);

			if (main_window)
				main_window->resize(width, height);

			return 0;
		}
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		}
		return ::DefWindowProcW(hWnd, msg, wParam, lParam);
	}
};

int main()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	window _window(L"Universal Engine Tool", 840, 920);
	main_window = &_window;

	if (dx.create_device(_window.get_window_handle()))
	{
		main_window = &_window;
		_window.run();
	}

	return 0;
}
