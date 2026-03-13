#pragma once

#include <Windows.h>
#include <string>

namespace Cloak {

    bool ExcludeFromCapture(HWND hwnd, bool exclude);  // SetWindowDisplayAffinity
    void HideFromTaskbar(HWND hwnd, bool hide);
    void SetTransparency(HWND hwnd, BYTE alpha);       // 0=invisible, 255=opaque
    void SetClickThrough(HWND hwnd, bool passthrough);
    void SetTopmost(HWND hwnd, bool topmost);
    bool SpoofProcessName(const std::wstring& fakeName); // PEB rewrite
    void SetWindowTitle(HWND hwnd, const std::wstring& title);
    void DisableCloseButton(HWND hwnd, bool disable);

}
