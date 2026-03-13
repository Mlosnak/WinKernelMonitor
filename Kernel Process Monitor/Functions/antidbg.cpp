#include "antidbg.h"
#include "../Usermodecomm/kmio.h"
#include <TlHelp32.h>
#include <winternl.h>
#include <sddl.h>

#pragma comment(lib, "Advapi32.lib")

static const std::vector<std::wstring> g_REToolNames = {
    L"x64dbg.exe",  L"x32dbg.exe",  L"ollydbg.exe",
    L"ida.exe",     L"ida64.exe",    L"idaq.exe",      L"idaq64.exe",
    L"windbg.exe",  L"windbgx.exe",  L"kd.exe",
    L"cdb.exe",     L"ntsd.exe",
    L"radare2.exe", L"r2.exe",
    L"ghidra.exe",  L"ghidraRun.exe",
    L"dnspy.exe",   L"dnSpy.exe",
    L"cheatengine-x86_64.exe", L"cheatengine.exe",
    L"processhacker.exe", L"procmon.exe", L"procmon64.exe",
    L"apimonitor.exe",
    L"scylla.exe",  L"scylla_x64.exe", L"scylla_x86.exe",
    L"de4dot.exe",
    L"httpdebugger.exe",
    L"fiddler.exe",
    L"wireshark.exe",
    L"pestudio.exe",
    L"die.exe",     L"detectiteasy.exe",
    L"hxd.exe",     L"HxD.exe",
    L"reclass.exe", L"ReClass.NET.exe",
    L"immunity debugger.exe",
};

const std::vector<std::wstring>& AntiDbg::GetREToolNames()
{
    return g_REToolNames;
}

bool AntiDbg::IsBeingDebugged()
{
    if (IsDebuggerPresent())
        return true;

    BOOL remoteDbg = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDbg) && remoteDbg)
        return true;

    // ProcessDebugPort
    typedef LONG(NTAPI* pNtQueryInfoProc)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        auto fn = (pNtQueryInfoProc)GetProcAddress(ntdll, "NtQueryInformationProcess");
        if (fn) {
            ULONG_PTR debugPort = 0;
            LONG st = fn(GetCurrentProcess(), 7 /*ProcessDebugPort*/,
                &debugPort, sizeof(debugPort), nullptr);
            if (st >= 0 && debugPort != 0)
                return true;
        }
    }

    return false;
}

bool AntiDbg::ScanForRETools(std::vector<ReToolInfo>& found)
{
    found.clear();

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring exeName = pe.szExeFile;
            for (const auto& tool : g_REToolNames) {
                if (_wcsicmp(exeName.c_str(), tool.c_str()) == 0) {
                    ReToolInfo ri;
                    ri.Name = exeName;
                    ri.Pid  = pe.th32ProcessID;

                    ri.IsDebugger = false;
                    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                    if (hProc) {
                        // Could check if it's debugging us specifically
                        CloseHandle(hProc);
                    }

                    found.push_back(std::move(ri));
                    break;
                }
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return true;
}

bool AntiDbg::HideFromDebugger()
{
    typedef LONG(NTAPI* pNtSetInfoThread)(HANDLE, ULONG, PVOID, ULONG);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    auto fn = (pNtSetInfoThread)GetProcAddress(ntdll, "NtSetInformationThread");
    if (!fn) return false;

    LONG status = fn(GetCurrentThread(), 0x11 /*ThreadHideFromDebugger*/, nullptr, 0);
    return (status >= 0);
}

bool AntiDbg::EnableKernelProtection(HANDLE hDriver)
{
    if (!hDriver || hDriver == INVALID_HANDLE_VALUE)
        return false;

    ULONG pid = GetCurrentProcessId();
    DWORD bytes = 0;
    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_KMPROCMON_SET_PROTECTED_PID,
        &pid, sizeof(pid),
        nullptr, 0,
        &bytes, nullptr);
    return ok != FALSE;
}

bool AntiDbg::QueryTokenInfo(DWORD pid, TokenInfo& info)
{
    info = {};

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return false;

    HANDLE hToken = nullptr;
    if (!OpenProcessToken(hProc, TOKEN_QUERY, &hToken)) {
        CloseHandle(hProc);
        return false;
    }

    TOKEN_ELEVATION elev{};
    DWORD retLen = 0;
    if (GetTokenInformation(hToken, TokenElevation, &elev, sizeof(elev), &retLen))
        info.Elevated = (elev.TokenIsElevated != 0);

    BYTE buf[256] = {};
    if (GetTokenInformation(hToken, TokenIntegrityLevel, buf, sizeof(buf), &retLen)) {
        auto* til = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(buf);
        DWORD* subAuth = GetSidSubAuthority(til->Label.Sid,
            (DWORD)(UCHAR)(*GetSidSubAuthorityCount(til->Label.Sid) - 1));
        info.IntegrityLevel = *subAuth;

        if (*subAuth >= SECURITY_MANDATORY_SYSTEM_RID)
            info.IntegrityStr = L"System";
        else if (*subAuth >= SECURITY_MANDATORY_HIGH_RID)
            info.IntegrityStr = L"High";
        else if (*subAuth >= SECURITY_MANDATORY_MEDIUM_RID)
            info.IntegrityStr = L"Medium";
        else if (*subAuth >= SECURITY_MANDATORY_LOW_RID)
            info.IntegrityStr = L"Low";
        else
            info.IntegrityStr = L"Untrusted";
    }

    BYTE userBuf[512] = {};
    if (GetTokenInformation(hToken, TokenUser, userBuf, sizeof(userBuf), &retLen)) {
        auto* tu = reinterpret_cast<TOKEN_USER*>(userBuf);
        wchar_t name[128] = {}, domain[128] = {};
        DWORD nameLen = 128, domLen = 128;
        SID_NAME_USE snu;
        if (LookupAccountSidW(nullptr, tu->User.Sid, name, &nameLen, domain, &domLen, &snu)) {
            info.UserName = std::wstring(domain) + L"\\" + name;
        }
    }

    CloseHandle(hToken);
    CloseHandle(hProc);
    return true;
}
