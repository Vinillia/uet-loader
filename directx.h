#ifndef _DIRECT_X_H
#define _DIRECT_X_H

#include <d3d11.h>
#include <wrl.h>

class directx
{
	friend class window;

public:
	directx();
	~directx();

	inline ID3D11Device* get_device() const noexcept;
	inline ID3D11DeviceContext* get_device_context() const noexcept;
	inline IDXGISwapChain* get_swap_chain() const noexcept;
	inline ID3D11RenderTargetView* get_render_target() const noexcept;

	bool create_device(HWND hwnd);

protected:
	void shutdown();
	bool create_render_target();
	void destroy_render_target();

private:
	Microsoft::WRL::ComPtr<ID3D11Device> _device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> _device_context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> _swap_chain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _render_target;
};

inline ID3D11RenderTargetView* directx::get_render_target() const noexcept
{
	return _render_target.Get();
}

inline IDXGISwapChain* directx::get_swap_chain() const noexcept
{
	return _swap_chain.Get();
}

inline ID3D11DeviceContext* directx::get_device_context() const noexcept
{
	return _device_context.Get();
}

inline ID3D11Device* directx::get_device() const noexcept
{
	return _device.Get();
}

#endif // !_DIRECT_X_H