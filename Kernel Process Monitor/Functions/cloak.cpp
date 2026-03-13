#include "cloak.h"
#include <winternl.h>

#pragma comment(lib, "ntdll.lib")

typedef NTSTATUS(NTAPI* pNtQueryInformationProcess)(
    HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

bool Cloak::ExcludeFromCapture(HWND hwnd, bool exclude)
{
    // WDA_EXCLUDEFROMCAPTURE (Win10 2004+), fallback to WDA_MONITOR
    DWORD affinity = exclude ? 0x00000011 /*WDA_EXCLUDEFROMCAPTURE*/ : WDA_NONE;
    if (!SetWindowDisplayAffinity(hwnd, affinity)) {
        if (exclude) {
            affinity = WDA_MONITOR;
            return SetWindowDisplayAffinity(hwnd, affinity) != FALSE;
        }
        return false;
    }
    return true;
}

void Cloak::HideFromTaskbar(HWND hwnd, bool hide)
{
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    if (hide) {
        exStyle &= ~WS_EX_APPWINDOW;
        exStyle |= WS_EX_TOOLWINDOW;
    }
    else {
        exStyle |= WS_EX_APPWINDOW;
        exStyle &= ~WS_EX_TOOLWINDOW;
    }

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(hwnd, SW_SHOW);
}

void Cloak::SetTransparency(HWND hwnd, BYTE alpha)
{
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED;
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);
    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
}

void Cloak::SetClickThrough(HWND hwnd, bool passthrough)
{
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (passthrough) {
        exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
    }
    else {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);
}

void Cloak::SetTopmost(HWND hwnd, bool topmost)
{
    SetWindowPos(
        hwnd,
        topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );
}

bool Cloak::SpoofProcessName(const std::wstring& fakeName)
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return false;

    auto fnQuery = reinterpret_cast<pNtQueryInformationProcess>(
        GetProcAddress(hNtdll, "NtQueryInformationProcess"));
    if (!fnQuery) return false;

    PROCESS_BASIC_INFORMATION pbi{};
    ULONG retLen = 0;
    NTSTATUS status = fnQuery(
        GetCurrentProcess(),
        ProcessBasicInformation,
        &pbi,
        sizeof(pbi),
        &retLen
    );
    if (status != 0) return false;

    PEB* pPeb = pbi.PebBaseAddress;
    if (!pPeb || !pPeb->ProcessParameters) return false;

    RTL_USER_PROCESS_PARAMETERS* params = pPeb->ProcessParameters;

    USHORT byteLen = static_cast<USHORT>(fakeName.size() * sizeof(wchar_t));
    if (byteLen <= params->CommandLine.MaximumLength) {
        memcpy(params->CommandLine.Buffer, fakeName.c_str(), byteLen);
        params->CommandLine.Length = byteLen;
        if (byteLen + sizeof(wchar_t) <= params->CommandLine.MaximumLength) {
            params->CommandLine.Buffer[fakeName.size()] = L'\0';
        }
    }

    if (byteLen <= params->ImagePathName.MaximumLength) {
        memcpy(params->ImagePathName.Buffer, fakeName.c_str(), byteLen);
        params->ImagePathName.Length = byteLen;
        if (byteLen + sizeof(wchar_t) <= params->ImagePathName.MaximumLength) {
            params->ImagePathName.Buffer[fakeName.size()] = L'\0';
        }
    }

    return true;
}

void Cloak::SetWindowTitle(HWND hwnd, const std::wstring& title)
{
    SetWindowTextW(hwnd, title.c_str());
}

void Cloak::DisableCloseButton(HWND hwnd, bool disable)
{
    HMENU hMenu = GetSystemMenu(hwnd, FALSE);
    if (!hMenu) return;

    if (disable) {
        EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
    }
    else {
        EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
    }
}
