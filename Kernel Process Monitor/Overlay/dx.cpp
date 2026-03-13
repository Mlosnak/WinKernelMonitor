#include "dx.h"

static void CreateRenderTarget(DxState* dx)
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    dx->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        dx->pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &dx->pRenderTarget);
        pBackBuffer->Release();
    }
}

static void DestroyRenderTarget(DxState* dx)
{
    if (dx->pRenderTarget) {
        dx->pRenderTarget->Release();
        dx->pRenderTarget = nullptr;
    }
}

bool DxCreate(HWND hwnd, DxState* dx)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    dx->Width  = static_cast<UINT>(rc.right - rc.left);
    dx->Height = static_cast<UINT>(rc.bottom - rc.top);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = dx->Width;
    sd.BufferDesc.Height                  = dx->Height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        levels,
        _countof(levels),
        D3D11_SDK_VERSION,
        &sd,
        &dx->pSwapChain,
        &dx->pDevice,
        &featureLevel,
        &dx->pContext
    );

    if (FAILED(hr)) return false;

    CreateRenderTarget(dx);
    return true;
}

void DxDestroy(DxState* dx)
{
    DestroyRenderTarget(dx);
    if (dx->pSwapChain)  { dx->pSwapChain->Release();  dx->pSwapChain  = nullptr; }
    if (dx->pContext)    { dx->pContext->Release();    dx->pContext    = nullptr; }
    if (dx->pDevice)     { dx->pDevice->Release();     dx->pDevice     = nullptr; }
}

void DxResize(DxState* dx, UINT w, UINT h)
{
    if (!dx->pSwapChain || (w == 0 && h == 0)) return;

    DestroyRenderTarget(dx);
    dx->pSwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    dx->Width  = w;
    dx->Height = h;
    CreateRenderTarget(dx);
}

void DxPresent(DxState* dx)
{
    dx->pSwapChain->Present(1, 0);
}
