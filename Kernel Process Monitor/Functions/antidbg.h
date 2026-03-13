#pragma once

#include <Windows.h>
#include <string>
#include <vector>

struct ReToolInfo {
    std::wstring    Name;
    DWORD           Pid;
    bool            IsDebugger;
};

struct TokenInfo {
    bool    Elevated;
    DWORD   IntegrityLevel;     // SECURITY_MANDATORY_*_RID
    std::wstring IntegrityStr;
    std::wstring UserName;
};

namespace AntiDbg {

    bool IsBeingDebugged();
    bool ScanForRETools(std::vector<ReToolInfo>& found);
    bool HideFromDebugger();  // NtSetInformationThread(ThreadHideFromDebugger)
    bool EnableKernelProtection(HANDLE hDriver);
    bool QueryTokenInfo(DWORD pid, TokenInfo& info);

    const std::vector<std::wstring>& GetREToolNames();
}
