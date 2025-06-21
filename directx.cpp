#include "directx.h"
#include <string>

using namespace Microsoft::WRL;

directx::directx()
{
}

directx::~directx()
{
    shutdown();
}

bool directx::create_device(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &_swap_chain, &_device, &featureLevel, &_device_context);

    if (result == DXGI_ERROR_UNSUPPORTED)
    {
        result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &_swap_chain, &_device, &featureLevel, &_device_context);
    }

    if (FAILED(result))
    {
        return false;
    }

    return create_render_target();
}

void directx::shutdown()
{
    destroy_render_target();
    _swap_chain = nullptr;
    _device = nullptr;
    _device_context = nullptr;
}

bool directx::create_render_target()
{
    ComPtr<ID3D11Texture2D> pBackBuffer;

    HRESULT hr = _swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr))
    {
        return false;
    }

    hr = _device->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &_render_target);
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void directx::destroy_render_target()
{
    _render_target = nullptr;
}