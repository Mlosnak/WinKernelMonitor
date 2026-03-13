#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include "../../Shared/KmProcMonShared.h"
#include "cert.h"
#include "pe.h"
#include "antidbg.h"

enum class Risk { Clean = 0, Low, Medium, High, Critical };

struct RiskColor4 { float r, g, b, a; };

struct ProcReport {
    KMPROCMON_EVENT_TYPE    EventType;
    DWORD                   Pid;
    DWORD                   PPid;
    std::wstring            Image;
    std::wstring            CmdLine;
    CertResult              Sig;
    Risk                    Level;
    std::vector<std::wstring>   Flags;
    std::vector<PeModule>       Modules;
    LARGE_INTEGER           Ts;
    LONGLONG                UptimeSeconds = 0;
    TokenInfo               Token;
    bool                    IsRETool = false;
};

class ThreatEngine {
public:
    ThreatEngine();

    ProcReport  Evaluate(const KMPROCMON_EVENT& ev);
    void        SetDeepScan(bool on) { m_deep = on; }

    static const wchar_t*   RiskStr(Risk r);
    static RiskColor4       RiskRgb(Risk r);

private:
    bool    IsSuspicious(const std::wstring& path);
    bool    IsSystem(const std::wstring& path);
    Risk    Score(const std::vector<std::wstring>& flags, CertResult sig);

    std::vector<std::wstring>   m_dirs;
    bool                        m_deep = false;
};
