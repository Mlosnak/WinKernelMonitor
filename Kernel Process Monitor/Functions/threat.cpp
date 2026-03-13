#include "threat.h"
#include "../Logs/dbglog.h"
#include <algorithm>
#include <format>
#include <ShlObj.h>

#pragma comment(lib, "Shell32.lib")

ThreatEngine::ThreatEngine()
{
    WCHAR tmp[MAX_PATH]{};
    WCHAR localAd[MAX_PATH]{};
    WCHAR roamAd[MAX_PATH]{};

    GetTempPathW(MAX_PATH, tmp);
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAd);
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, roamAd);

    m_dirs.push_back(tmp);
    m_dirs.push_back(localAd);
    m_dirs.push_back(roamAd);
    m_dirs.push_back(L"C:\\Users\\Public");
    m_dirs.push_back(L"C:\\ProgramData");

    for (auto& d : m_dirs) {
        if (!d.empty() && d.back() == L'\\') d.pop_back();
        std::transform(d.begin(), d.end(), d.begin(), ::towlower);
    }
}

bool ThreatEngine::IsSuspicious(const std::wstring& path)
{
    if (path.empty()) return false;
    std::wstring lw = path;
    std::transform(lw.begin(), lw.end(), lw.begin(), ::towlower);

    for (const auto& d : m_dirs)
        if (lw.find(d) == 0) return true;

    if (lw.find(L"\\$recycle.bin\\") != std::wstring::npos) return true;
    if (lw.find(L"\\downloads\\") != std::wstring::npos)    return true;

    return false;
}

bool ThreatEngine::IsSystem(const std::wstring& path)
{
    if (path.empty()) return false;
    std::wstring lw = path;
    std::transform(lw.begin(), lw.end(), lw.begin(), ::towlower);
    return (lw.find(L"c:\\windows\\system32\\") == 0 ||
            lw.find(L"c:\\windows\\syswow64\\") == 0);
}

Risk ThreatEngine::Score(const std::vector<std::wstring>& flags, CertResult sig)
{
    int pts = 0;
    for (const auto& f : flags) {
        if (f.find(L"SUSPICIOUS_DIR") != std::wstring::npos) pts += 30;
        if (f.find(L"RECYCLE_BIN")    != std::wstring::npos) pts += 40;
        if (f.find(L"NO_IMAGE")       != std::wstring::npos) pts += 20;
    }

    switch (sig) {
    case CertResult::Unsigned:      pts += 25; break;
    case CertResult::BadSignature:  pts += 50; break;
    case CertResult::UntrustedRoot: pts += 35; break;
    case CertResult::Expired:       pts += 15; break;
    case CertResult::Failed:        pts += 10; break;
    default: break;
    }

    if (pts >= 80) return Risk::Critical;
    if (pts >= 50) return Risk::High;
    if (pts >= 25) return Risk::Medium;
    if (pts > 0)   return Risk::Low;
    return Risk::Clean;
}

ProcReport ThreatEngine::Evaluate(const KMPROCMON_EVENT& ev)
{
    ProcReport rpt{};
    rpt.EventType = ev.Type;
    rpt.Pid    = ev.ProcessId;
    rpt.PPid   = ev.ParentProcessId;
    rpt.Image  = ev.ImagePath;
    rpt.CmdLine = ev.CommandLine;
    rpt.Ts     = ev.Timestamp;

    if (ev.Type == EventProcessCreate && ev.ProcessId != 0) {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ev.ProcessId);
        if (hProc) {
            FILETIME creation{}, exitFt{}, kernelFt{}, userFt{};
            if (GetProcessTimes(hProc, &creation, &exitFt, &kernelFt, &userFt)) {
                FILETIME now; GetSystemTimeAsFileTime(&now);
                ULARGE_INTEGER uNow, uCreate;
                uNow.LowPart    = now.dwLowDateTime;
                uNow.HighPart   = now.dwHighDateTime;
                uCreate.LowPart = creation.dwLowDateTime;
                uCreate.HighPart = creation.dwHighDateTime;
                if (uNow.QuadPart > uCreate.QuadPart)
                    rpt.UptimeSeconds = (LONGLONG)((uNow.QuadPart - uCreate.QuadPart) / 10000000ULL);
            }
            CloseHandle(hProc);
        }

        AntiDbg::QueryTokenInfo(ev.ProcessId, rpt.Token);

        if (!rpt.Image.empty()) {
            std::wstring lowerImg = rpt.Image;
            std::transform(lowerImg.begin(), lowerImg.end(), lowerImg.begin(), ::towlower);
            auto pos = lowerImg.find_last_of(L"\\/");
            std::wstring fname = (pos != std::wstring::npos) ? lowerImg.substr(pos + 1) : lowerImg;
            for (const auto& tool : AntiDbg::GetREToolNames()) {
                std::wstring lowerTool = tool;
                std::transform(lowerTool.begin(), lowerTool.end(), lowerTool.begin(), ::towlower);
                if (fname == lowerTool) {
                    rpt.IsRETool = true;
                    rpt.Flags.push_back(L"RE_TOOL_DETECTED");
                    break;
                }
            }
        }
    }

    if (IsSystem(rpt.Image)) {
        rpt.Sig   = CertResult::Trusted;
        rpt.Level = Risk::Clean;
        return rpt;
    }

    if (rpt.Image.empty()) {
        rpt.Flags.push_back(L"NO_IMAGE");
    }
    else {
        if (IsSuspicious(rpt.Image)) {
            std::wstring lw = rpt.Image;
            std::transform(lw.begin(), lw.end(), lw.begin(), ::towlower);
            if (lw.find(L"\\$recycle.bin\\") != std::wstring::npos)
                rpt.Flags.push_back(L"RECYCLE_BIN");
            else
                rpt.Flags.push_back(L"SUSPICIOUS_DIR");
        }

        rpt.Sig = Cert::Check(rpt.Image);
        if (rpt.Sig != CertResult::Trusted)
            rpt.Flags.push_back(std::format(L"SIG_{}", Cert::Str(rpt.Sig)));
    }

    if (m_deep && ev.ProcessId != 0)
        Pe::Walk(ev.ProcessId, rpt.Modules);

    rpt.Level = Score(rpt.Flags, rpt.Sig);
    return rpt;
}

const wchar_t* ThreatEngine::RiskStr(Risk r)
{
    switch (r) {
    case Risk::Clean:    return L"CLEAN";
    case Risk::Low:      return L"LOW";
    case Risk::Medium:   return L"MEDIUM";
    case Risk::High:     return L"HIGH";
    case Risk::Critical: return L"CRITICAL";
    default:             return L"?";
    }
}

RiskColor4 ThreatEngine::RiskRgb(Risk r)
{
    switch (r) {
    case Risk::Clean:    return { 0.2f, 0.9f, 0.3f, 1.f };
    case Risk::Low:      return { 0.4f, 0.8f, 0.9f, 1.f };
    case Risk::Medium:   return { 1.0f, 0.8f, 0.2f, 1.f };
    case Risk::High:     return { 1.0f, 0.3f, 0.2f, 1.f };
    case Risk::Critical: return { 1.0f, 0.1f, 0.6f, 1.f };
    default:             return { 0.7f, 0.7f, 0.7f, 1.f };
    }
}
