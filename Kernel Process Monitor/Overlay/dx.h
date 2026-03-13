#pragma once

#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

struct DxState {
    ID3D11Device*           pDevice         = nullptr;
    ID3D11DeviceContext*    pContext         = nullptr;
    IDXGISwapChain*         pSwapChain      = nullptr;
    ID3D11RenderTargetView* pRenderTarget   = nullptr;
    UINT                    Width           = 0;
    UINT                    Height          = 0;
};

bool    DxCreate(HWND hwnd, DxState* dx);
void    DxDestroy(DxState* dx);
void    DxResize(DxState* dx, UINT w, UINT h);
void    DxPresent(DxState* dx);
