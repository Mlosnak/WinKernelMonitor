#include "overlay.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

HWND OverlayCreate(HINSTANCE hInst, int w, int h, const wchar_t* title)
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"KmProcMonWnd";

    RegisterClassExW(&wc);

    //
    // Borderless popup with thick frame for resize
    //
    DWORD style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    int x  = (sx - w) / 2;
    int y  = (sy - h) / 2;

    HWND hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        title,
        style,
        x, y,
        w, h,
        nullptr, nullptr,
        hInst,
        nullptr
    );

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    return hwnd;
}

void OverlayDestroy(HWND hwnd)
{
    DestroyWindow(hwnd);
    UnregisterClassW(L"KmProcMonWnd", GetModuleHandleW(nullptr));
}
