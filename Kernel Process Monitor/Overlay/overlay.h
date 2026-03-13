#pragma once

#include <Windows.h>

HWND    OverlayCreate(HINSTANCE hInst, int w, int h, const wchar_t* title);
void    OverlayDestroy(HWND hwnd);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
