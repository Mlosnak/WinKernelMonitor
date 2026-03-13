#include "render.h"
#include "../Logs/dbglog.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"

#include <format>
#include <algorithm>
#include <string>
#include <shellapi.h>
#include <TlHelp32.h>
#include <cstdlib>
#include <ctime>
#include <fstream>

#pragma comment(lib, "Shell32.lib")

static void ApplyModernTheme()
{
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding    = 0.f;
    st.ChildRounding     = 6.f;
    st.FrameRounding     = 4.f;
    st.GrabRounding      = 4.f;
    st.PopupRounding     = 6.f;
    st.ScrollbarRounding = 6.f;
    st.TabRounding       = 4.f;
    st.WindowPadding     = { 8, 8 };
    st.FramePadding      = { 8, 5 };
    st.ItemSpacing       = { 8, 6 };
    st.ItemInnerSpacing  = { 6, 4 };
    st.ScrollbarSize     = 12.f;
    st.GrabMinSize       = 8.f;
    st.WindowBorderSize  = 0.f;
    st.ChildBorderSize   = 1.f;
    st.PopupBorderSize   = 1.f;
    st.FrameBorderSize   = 0.f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg]             = { 0.07f, 0.07f, 0.09f, 1.f };
    c[ImGuiCol_ChildBg]              = { 0.08f, 0.08f, 0.11f, 1.f };
    c[ImGuiCol_PopupBg]              = { 0.09f, 0.09f, 0.12f, 0.98f };
    c[ImGuiCol_Border]               = { 0.16f, 0.18f, 0.22f, 0.60f };
    c[ImGuiCol_FrameBg]              = { 0.12f, 0.12f, 0.16f, 1.f };
    c[ImGuiCol_FrameBgHovered]       = { 0.16f, 0.16f, 0.22f, 1.f };
    c[ImGuiCol_FrameBgActive]        = { 0.20f, 0.20f, 0.28f, 1.f };
    c[ImGuiCol_TitleBg]              = { 0.06f, 0.06f, 0.08f, 1.f };
    c[ImGuiCol_TitleBgActive]        = { 0.06f, 0.06f, 0.08f, 1.f };
    c[ImGuiCol_ScrollbarBg]          = { 0.06f, 0.06f, 0.08f, 0.5f };
    c[ImGuiCol_ScrollbarGrab]        = { 0.22f, 0.22f, 0.28f, 1.f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.28f, 0.28f, 0.36f, 1.f };
    c[ImGuiCol_ScrollbarGrabActive]  = { 0.16f, 0.72f, 0.44f, 1.f };
    c[ImGuiCol_CheckMark]            = { 0.16f, 0.80f, 0.50f, 1.f };
    c[ImGuiCol_SliderGrab]           = { 0.16f, 0.72f, 0.44f, 1.f };
    c[ImGuiCol_SliderGrabActive]     = { 0.20f, 0.85f, 0.55f, 1.f };
    c[ImGuiCol_Button]               = { 0.12f, 0.45f, 0.30f, 1.f };
    c[ImGuiCol_ButtonHovered]        = { 0.16f, 0.60f, 0.38f, 1.f };
    c[ImGuiCol_ButtonActive]         = { 0.20f, 0.72f, 0.46f, 1.f };
    c[ImGuiCol_Header]               = { 0.11f, 0.11f, 0.15f, 1.f };
    c[ImGuiCol_HeaderHovered]        = { 0.14f, 0.38f, 0.26f, 1.f };
    c[ImGuiCol_HeaderActive]         = { 0.16f, 0.46f, 0.30f, 1.f };
    c[ImGuiCol_Separator]            = { 0.16f, 0.18f, 0.22f, 0.60f };
    c[ImGuiCol_Tab]                  = { 0.10f, 0.10f, 0.14f, 1.f };
    c[ImGuiCol_TabHovered]           = { 0.16f, 0.60f, 0.38f, 1.f };
    c[ImGuiCol_TabActive]            = { 0.12f, 0.45f, 0.30f, 1.f };
    c[ImGuiCol_TableHeaderBg]        = { 0.09f, 0.09f, 0.13f, 1.f };
    c[ImGuiCol_TableBorderStrong]    = { 0.16f, 0.16f, 0.22f, 1.f };
    c[ImGuiCol_TableBorderLight]     = { 0.11f, 0.11f, 0.15f, 1.f };
    c[ImGuiCol_TableRowBg]           = { 0, 0, 0, 0 };
    c[ImGuiCol_TableRowBgAlt]        = { 1, 1, 1, 0.02f };
    c[ImGuiCol_Text]                 = { 0.88f, 0.90f, 0.92f, 1.f };
    c[ImGuiCol_TextDisabled]         = { 0.45f, 0.47f, 0.52f, 1.f };
}

static std::wstring FtToLocal(const LARGE_INTEGER& ts)
{
    FILETIME ft;
    ft.dwLowDateTime  = ts.LowPart;
    ft.dwHighDateTime  = static_cast<DWORD>(ts.HighPart);
    FILETIME localFt;
    FileTimeToLocalFileTime(&ft, &localFt);
    SYSTEMTIME st;
    FileTimeToSystemTime(&localFt, &st);
    return std::format(L"{:02d}:{:02d}:{:02d}.{:03d}",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

static std::string WtoA(const std::wstring& ws)
{
    if (ws.empty()) return {};
    int sz = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    std::string out(sz, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), out.data(), sz, nullptr, nullptr);
    return out;
}

static std::wstring AtoW(const std::string& s)
{
    if (s.empty()) return {};
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(sz, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), sz);
    return out;
}

static std::wstring CleanNtPath(const std::wstring& path)
{
    std::wstring p = path;
    if (p.starts_with(L"\\??\\")) p = p.substr(4);
    else if (p.starts_with(L"\\\\?\\")) p = p.substr(4);
    return p;
}

static std::string FileName(const std::wstring& path)
{
    if (path.empty()) return "<unknown>";
    auto clean = CleanNtPath(path);
    auto pos = clean.find_last_of(L"\\/");
    return WtoA(pos == std::wstring::npos ? clean : clean.substr(pos + 1));
}

static std::string FormatBytes(DWORD bytes)
{
    if (bytes >= 1048576) return std::format("{:.1f} MB", bytes / 1048576.0);
    if (bytes >= 1024)    return std::format("{:.1f} KB", bytes / 1024.0);
    return std::format("{} B", bytes);
}

static const char* EventTypeStr(KMPROCMON_EVENT_TYPE t)
{
    switch (t) {
    case EventProcessCreate: return "NEW";
    case EventProcessExit:   return "EXIT";
    case EventThreadCreate:  return "THR+";
    case EventThreadExit:    return "THR-";
    default:                 return "?";
    }
}

static ImVec4 EventTypeColor(KMPROCMON_EVENT_TYPE t)
{
    switch (t) {
    case EventProcessCreate: return { 0.16f, 0.80f, 0.50f, 1.f };
    case EventProcessExit:   return { 0.85f, 0.35f, 0.30f, 1.f };
    case EventThreadCreate:  return { 0.40f, 0.70f, 0.90f, 1.f };
    case EventThreadExit:    return { 0.55f, 0.55f, 0.60f, 1.f };
    default:                 return { 0.50f, 0.50f, 0.50f, 1.f };
    }
}

// --- process interaction ---

static void SetStatusMsg(AppState& app, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(app.StatusMsg, sizeof(app.StatusMsg), fmt, args);
    va_end(args);
    app.StatusTimer = 5.f;
}

static bool KillProcessById(DWORD pid, AppState* app = nullptr)
{
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h) {
        DWORD err = GetLastError();
        DbgLog::Write(LogSev::Err, L"Kill",
            std::format(L"OpenProcess(TERMINATE) failed for PID={} err=0x{:08X}", pid, err));
        if (app) {
            if (err == ERROR_ACCESS_DENIED)
                SetStatusMsg(*app, "Kill PID %lu FAILED: Access Denied (run as Admin)", pid);
            else if (err == ERROR_INVALID_PARAMETER)
                SetStatusMsg(*app, "Kill PID %lu FAILED: Process already exited", pid);
            else
                SetStatusMsg(*app, "Kill PID %lu FAILED: error 0x%08X", pid, err);
        }
        return false;
    }
    BOOL ok = TerminateProcess(h, 1);
    DWORD err = GetLastError();
    CloseHandle(h);
    if (!ok) {
        DbgLog::Write(LogSev::Err, L"Kill",
            std::format(L"TerminateProcess failed for PID={} err=0x{:08X}", pid, err));
        if (app) SetStatusMsg(*app, "Kill PID %lu FAILED: TerminateProcess err 0x%08X", pid, err);
        return false;
    }
    DbgLog::Write(LogSev::Ok, L"Kill", std::format(L"Successfully killed PID={}", pid));
    if (app) SetStatusMsg(*app, "Killed PID %lu successfully", pid);
    return true;
}

static void OpenFileLocation(const std::wstring& path, AppState* app = nullptr)
{
    if (path.empty()) return;
    std::wstring clean = CleanNtPath(path);
    DbgLog::Write(LogSev::Ok, L"OpenLoc",
        std::format(L"Opening location: {}", clean));
    std::wstring cmd = L"/select,\"" + clean + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", cmd.c_str(), nullptr, SW_SHOWNORMAL);
    if (app) SetStatusMsg(*app, "Opened: %s", WtoA(clean).c_str());
}

static bool SuspendResumeProcess(DWORD pid, bool suspend, AppState* app = nullptr)
{
    typedef LONG(NTAPI* pNtSuspend)(HANDLE);
    typedef LONG(NTAPI* pNtResume)(HANDLE);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;
    auto fnSuspend = (pNtSuspend)GetProcAddress(ntdll, "NtSuspendProcess");
    auto fnResume  = (pNtResume)GetProcAddress(ntdll, "NtResumeProcess");
    if (!fnSuspend || !fnResume) return false;

    HANDLE h = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!h) {
        DWORD err = GetLastError();
        DbgLog::Write(LogSev::Err, L"SusRes",
            std::format(L"OpenProcess failed for PID={} err=0x{:08X}", pid, err));
        if (app) SetStatusMsg(*app, "%s PID %lu FAILED: err 0x%08X",
            suspend ? "Suspend" : "Resume", pid, err);
        return false;
    }
    LONG status = suspend ? fnSuspend(h) : fnResume(h);
    CloseHandle(h);
    bool ok = (status >= 0);
    if (ok) {
        DbgLog::Write(LogSev::Ok, L"SusRes",
            std::format(L"{} PID={}", suspend ? L"Suspended" : L"Resumed", pid));
        if (app) SetStatusMsg(*app, "%s PID %lu OK", suspend ? "Suspended" : "Resumed", pid);
    } else {
        if (app) SetStatusMsg(*app, "%s PID %lu FAILED: NTSTATUS 0x%08X",
            suspend ? "Suspend" : "Resume", pid, (unsigned)status);
    }
    return ok;
}

static void ExportEventLog(const AppState& app)
{
    SYSTEMTIME now; GetLocalTime(&now);
    char fname[128];
    snprintf(fname, sizeof(fname), "kmprocmon_export_%04d%02d%02d_%02d%02d%02d.csv",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    std::ofstream ofs(fname);
    if (!ofs) return;
    ofs << "Time,Type,PID,PPID,Risk,Image,CmdLine,Flags\n";
    for (const auto& r : app.EventLog) {
        ofs << WtoA(FtToLocal(r.Ts)) << ","
            << EventTypeStr(r.EventType) << ","
            << r.Pid << "," << r.PPid << ","
            << WtoA(ThreatEngine::RiskStr(r.Level)) << ","
            << "\"" << WtoA(CleanNtPath(r.Image)) << "\","
            << "\"" << WtoA(r.CmdLine) << "\",";
        for (size_t i = 0; i < r.Flags.size(); i++) {
            if (i > 0) ofs << "|";
            ofs << WtoA(r.Flags[i]);
        }
        ofs << "\n";
    }
    ofs.close();
}

static const char* kRandomNames[] = {
    "svchost.exe", "explorer.exe", "csrss.exe", "lsass.exe",
    "winlogon.exe", "services.exe", "System", "dwm.exe",
    "conhost.exe", "RuntimeBroker.exe", "taskhostw.exe",
    "sihost.exe", "fontdrvhost.exe", "ctfmon.exe",
    "SearchIndexer.exe", "WmiPrvSE.exe", "spoolsv.exe",
    "audiodg.exe", "SecurityHealthService.exe", "MsMpEng.exe",
    "chrome.exe", "firefox.exe", "notepad.exe", "cmd.exe",
    "powershell.exe", "wininit.exe", "smss.exe", "dllhost.exe",
    "TotallyLegit.exe", "NotAVirus.exe", "trustme.exe",
    "DefinitelyWindows.exe", "UpdateHelper.exe"
};
static const int kRandomNameCount = sizeof(kRandomNames) / sizeof(kRandomNames[0]);

static void ApplyRandomSpoof(AppState& app)
{
    static bool seeded = false;
    if (!seeded) { srand((unsigned)time(nullptr)); seeded = true; }
    int idx = rand() % kRandomNameCount;
    const char* name = kRandomNames[idx];
    strncpy_s(app.SpoofName, name, sizeof(app.SpoofName) - 1);
    Cloak::SpoofProcessName(AtoW(name));
    Cloak::SetWindowTitle(app.hWnd, AtoW(name));
}

static void EnumProcessModules(DWORD pid, std::vector<PeModule>& out)
{
    out.clear();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return;

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    if (Module32FirstW(snap, &me)) {
        do {
            PeModule m;
            m.Name = me.szModule;
            m.Path = me.szExePath;
            m.Base = me.hModule;
            m.Size = me.modBaseSize;
            out.push_back(std::move(m));
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
}

static void CopyToClipboard(const std::string& text)
{
    if (text.empty()) return;
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hg) {
        memcpy(GlobalLock(hg), text.c_str(), text.size() + 1);
        GlobalUnlock(hg);
        SetClipboardData(CF_TEXT, hg);
    }
    CloseClipboard();
}

static void DrawStats(AppState& app)
{
    auto& s = app.Stats;
    LONGLONG sec = s.UptimeTicks / 10000000LL;
    LONGLONG hrs = sec / 3600, mins = (sec % 3600) / 60, secs = sec % 60;

    ImVec4 dim  = { 0.45f, 0.48f, 0.52f, 1.f };
    ImVec4 acc  = { 0.16f, 0.80f, 0.50f, 1.f };
    ImVec4 warn = { 1.0f, 0.4f, 0.3f, 1.f };

    ImGui::TextColored(dim, "UPTIME");
    ImGui::SameLine(160.f);
    ImGui::TextColored(acc, "%lldh %02lldm %02llds", hrs, mins, secs);

    ImGui::Spacing();
    ImGui::TextColored(dim, "PROCESSES");
    ImGui::Indent(10.f);
    LONGLONG active = s.TotalProcessCreated - s.TotalProcessExited;
    ImGui::Text("Created: %lld", s.TotalProcessCreated);
    ImGui::Text("Exited:  %lld", s.TotalProcessExited);
    ImGui::TextColored(acc, "Active:  %lld", active > 0 ? active : 0);
    ImGui::Unindent(10.f);

    ImGui::Spacing();
    ImGui::TextColored(dim, "THREADS");
    ImGui::Indent(10.f);
    ImGui::Text("Created: %lld", s.TotalThreadCreated);
    ImGui::Text("Exited:  %lld", s.TotalThreadExited);
    ImGui::Unindent(10.f);

    ImGui::Spacing();
    ImGui::TextColored(dim, "BUFFER");
    ImGui::Indent(10.f);
    ImGui::Text("Log: %d / %d", (int)app.EventLog.size(), app.MaxLogEntries);
    if (s.EventsDropped > 0)
        ImGui::TextColored(warn, "Dropped: %lld", s.EventsDropped);
    else
        ImGui::TextColored(acc, "Dropped: 0");
    ImGui::Unindent(10.f);

    ImGui::Spacing();
    ImGui::TextColored(dim, "THREAT SUMMARY");
    ImGui::Indent(10.f);
    int crit = 0, high = 0, med = 0, low = 0;
    for (const auto& e : app.EventLog) {
        switch (e.Level) {
        case Risk::Critical: crit++; break;
        case Risk::High:     high++; break;
        case Risk::Medium:   med++;  break;
        case Risk::Low:      low++;  break;
        default: break;
        }
    }
    if (crit) ImGui::TextColored({1.f, 0.1f, 0.6f, 1.f}, "Critical: %d", crit);
    if (high) ImGui::TextColored({1.f, 0.3f, 0.2f, 1.f}, "High: %d", high);
    if (med)  ImGui::TextColored({1.f, 0.8f, 0.2f, 1.f}, "Medium: %d", med);
    if (low)  ImGui::TextColored({0.4f, 0.8f, 0.9f, 1.f}, "Low: %d", low);
    if (!crit && !high && !med && !low) ImGui::TextColored(acc, "All clear");
    ImGui::Unindent(10.f);
}

static void DrawConfig(AppState& app)
{
    bool changed = false;

    if (ImGui::Checkbox("Monitor Threads", &app.MonitorThreads)) changed = true;
    if (ImGui::Checkbox("Capture Command Line", &app.CaptureCmd)) changed = true;
    if (ImGui::Checkbox("Deep Scan (modules)", &app.DeepScan)) {}

    ImGui::Spacing();
    ImGui::SliderFloat("Poll (s)", &app.PollInterval, 0.1f, 5.0f, "%.1f");
    ImGui::SliderInt("Max Log", &app.MaxLogEntries, 100, 5000);

    if (changed && app.pDriver) {
        app.Cfg.MonitorThreads     = app.MonitorThreads ? TRUE : FALSE;
        app.Cfg.CaptureCommandLine = app.CaptureCmd ? TRUE : FALSE;
        app.pDriver->PushConfig(app.Cfg);
    }

    if (app.pEngine)
        app.pEngine->SetDeepScan(app.DeepScan);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float bw = (ImGui::GetContentRegionAvail().x - 16.f) / 3.f;
    if (ImGui::Button("Clear Log", ImVec2(bw, 0)))
        app.EventLog.clear();
    ImGui::SameLine();
    if (ImGui::Button("Flush Buffer", ImVec2(bw, 0))) {
        if (app.pDriver) app.pDriver->Flush();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export CSV", ImVec2(bw, 0))) {
        ExportEventLog(app);
        SetStatusMsg(app, "Exported %d events to CSV", (int)app.EventLog.size());
    }

    ImGui::Spacing();
    ImGui::Checkbox("Auto-scroll", &app.AutoScroll);
}

static void DrawCloak(AppState& app)
{
    if (ImGui::Checkbox("Exclude From Capture", &app.CloakCapture))
        Cloak::ExcludeFromCapture(app.hWnd, app.CloakCapture);
    if (ImGui::Checkbox("Hide From Taskbar", &app.CloakTaskbar))
        Cloak::HideFromTaskbar(app.hWnd, app.CloakTaskbar);
    if (ImGui::Checkbox("Always On Top", &app.CloakTopmost))
        Cloak::SetTopmost(app.hWnd, app.CloakTopmost);

    int alpha = app.CloakAlpha;
    if (ImGui::SliderInt("Opacity", &alpha, 30, 255)) {
        app.CloakAlpha = static_cast<BYTE>(alpha);
        Cloak::SetTransparency(app.hWnd, app.CloakAlpha);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored({ 0.45f, 0.48f, 0.52f, 1.f }, "PROCESS NAME SPOOF");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##spoof_input", "Enter new name...", app.SpoofName, sizeof(app.SpoofName));
    ImGui::Spacing();
    if (ImGui::Button("Apply Spoof", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        if (app.SpoofName[0] != '\0') {
            Cloak::SpoofProcessName(AtoW(app.SpoofName));
            Cloak::SetWindowTitle(app.hWnd, AtoW(app.SpoofName));
            DbgLog::Write(LogSev::Warn, L"Cloak",
                std::format(L"Process name spoofed to: {}", AtoW(app.SpoofName)));
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored({ 0.45f, 0.48f, 0.52f, 1.f }, "RANDOM NAME CHAOS");
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button,
        app.RandomSpoofMode ? ImVec4(0.55f, 0.15f, 0.15f, 1.f) : ImVec4(0.12f, 0.45f, 0.30f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        app.RandomSpoofMode ? ImVec4(0.75f, 0.2f, 0.2f, 1.f) : ImVec4(0.16f, 0.60f, 0.38f, 1.f));
    if (ImGui::Button(app.RandomSpoofMode ? "STOP Chaos" : "START Chaos",
        ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        app.RandomSpoofMode = !app.RandomSpoofMode;
        app.RandomSpoofTimer = 0.f;
        if (app.RandomSpoofMode) {
            ApplyRandomSpoof(app);
            SetStatusMsg(app, "Random name chaos ENABLED");
        } else {
            Cloak::SetWindowTitle(app.hWnd, L"Kernel Process Monitor");
            SetStatusMsg(app, "Random name chaos disabled");
        }
    }
    ImGui::PopStyleColor(2);

    if (app.RandomSpoofMode) {
        ImGui::SliderFloat("Interval (s)", &app.RandomSpoofInterval, 0.05f, 3.0f, "%.2f");
        ImGui::TextColored({ 0.8f, 0.6f, 0.2f, 1.f }, "Current: %s", app.SpoofName);

        float dt = ImGui::GetIO().DeltaTime;
        app.RandomSpoofTimer += dt;
        if (app.RandomSpoofTimer >= app.RandomSpoofInterval) {
            app.RandomSpoofTimer = 0.f;
            ApplyRandomSpoof(app);
        }
    }
}

static void DrawEventLog(AppState& app)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##filter", "Filter by name, PID, or path...",
        app.FilterText, sizeof(app.FilterText));
    ImGui::Spacing();

    int contextIdx = -1;

    if (ImGui::BeginTable("Events", 7,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Time",  ImGuiTableColumnFlags_WidthFixed, 90.f);
        ImGui::TableSetupColumn("Type",  ImGuiTableColumnFlags_WidthFixed, 44.f);
        ImGui::TableSetupColumn("PID",   ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("PPID",  ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("Risk",  ImGuiTableColumnFlags_WidthFixed, 62.f);
        ImGui::TableSetupColumn("Image", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        std::string filterLower;
        if (app.FilterText[0]) {
            filterLower = app.FilterText;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
        }

        for (int i = (int)app.EventLog.size() - 1; i >= 0; i--) {
            auto& r = app.EventLog[i];

            if (!filterLower.empty()) {
                std::string nm = FileName(r.Image);
                std::transform(nm.begin(), nm.end(), nm.begin(), ::tolower);
                std::string pid = std::to_string(r.Pid);
                std::string pa = WtoA(r.Image);
                std::transform(pa.begin(), pa.end(), pa.begin(), ::tolower);
                if (nm.find(filterLower) == std::string::npos &&
                    pid.find(filterLower) == std::string::npos &&
                    pa.find(filterLower) == std::string::npos)
                    continue;
            }

            auto rc = ThreatEngine::RiskRgb(r.Level);
            ImVec4 riskCol = { rc.r, rc.g, rc.b, rc.a };

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            char selId[32]; snprintf(selId, sizeof(selId), "##s%d", i);
            bool isSel = (app.SelectedIdx == i);
            if (ImGui::Selectable(selId, isSel,
                    ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 0)))
            {
                app.SelectedIdx = i;
                app.ShowDetails = true;
                EnumProcessModules(r.Pid, app.InspectModules);
            }
            if (ImGui::IsItemClicked(1))
                contextIdx = i;
            ImGui::SameLine();
            ImGui::TextUnformatted(WtoA(FtToLocal(r.Ts)).c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(EventTypeColor(r.EventType), "%s", EventTypeStr(r.EventType));

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%lu", r.Pid);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%lu", r.PPid);

            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(riskCol, "%s", WtoA(ThreatEngine::RiskStr(r.Level)).c_str());

            ImGui::TableSetColumnIndex(5);
            ImGui::TextColored(riskCol, "%s", FileName(r.Image).c_str());
            if (ImGui::IsItemHovered() && !r.Image.empty()) {
                ImGui::BeginTooltip();
                ImGui::Text("Path: %s", WtoA(CleanNtPath(r.Image)).c_str());
                if (!r.CmdLine.empty()) {
                    ImGui::Separator();
                    ImGui::TextWrapped("CMD: %s", WtoA(r.CmdLine).c_str());
                }
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(6);
            std::string flagStr;
            for (const auto& f : r.Flags) {
                if (!flagStr.empty()) flagStr += " | ";
                flagStr += WtoA(f);
            }
            if (!flagStr.empty())
                ImGui::TextColored({ 1.f, 0.6f, 0.2f, 1.f }, "%s", flagStr.c_str());
        }

        ImGui::EndTable();
    }

    if (contextIdx >= 0) {
        app.SelectedIdx = contextIdx;
        ImGui::OpenPopup("##ctx");
    }
    if (ImGui::BeginPopup("##ctx")) {
        if (app.SelectedIdx >= 0 && app.SelectedIdx < (int)app.EventLog.size()) {
            auto& sel = app.EventLog[app.SelectedIdx];
            ImGui::TextColored({ 0.45f, 0.48f, 0.52f, 1.f },
                "PID %lu  -  %s", sel.Pid, FileName(sel.Image).c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Kill Process")) {
                DWORD killPid = sel.Pid;
                if (KillProcessById(killPid, &app)) {
                    app.EventLog.erase(app.EventLog.begin() + app.SelectedIdx);
                    app.SelectedIdx = -1;
                    app.ShowDetails = false;
                }
            }
            if (ImGui::MenuItem("Suspend Process"))
                SuspendResumeProcess(sel.Pid, true, &app);
            if (ImGui::MenuItem("Resume Process"))
                SuspendResumeProcess(sel.Pid, false, &app);
            ImGui::Separator();
            if (ImGui::MenuItem("Open File Location", nullptr, false, !sel.Image.empty()))
                OpenFileLocation(sel.Image, &app);
            if (ImGui::MenuItem("Inspect DLLs")) {
                EnumProcessModules(sel.Pid, app.InspectModules);
                app.ShowDetails = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Path", nullptr, false, !sel.Image.empty()))
                CopyToClipboard(WtoA(CleanNtPath(sel.Image)));
            if (ImGui::MenuItem("Copy Command Line", nullptr, false, !sel.CmdLine.empty()))
                CopyToClipboard(WtoA(sel.CmdLine));
            if (ImGui::MenuItem("Copy PID"))
                CopyToClipboard(std::to_string(sel.Pid));
        }
        ImGui::EndPopup();
    }
}

static void DrawProcessDetails(AppState& app)
{
    if (!app.ShowDetails) return;
    if (app.SelectedIdx < 0 || app.SelectedIdx >= (int)app.EventLog.size()) {
        app.ShowDetails = false;
        return;
    }

    auto& sel = app.EventLog[app.SelectedIdx];

    ImGui::SetNextWindowSize(ImVec2(620, 440), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Process Details", &app.ShowDetails, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    std::string name = FileName(sel.Image);
    auto rc = ThreatEngine::RiskRgb(sel.Level);

    ImGui::TextColored({ 0.16f, 0.80f, 0.50f, 1.f }, "%s", name.c_str());
    ImGui::SameLine();
    ImGui::TextColored({ 0.45f, 0.48f, 0.52f, 1.f }, "(PID %lu)", sel.Pid);
    ImGui::SameLine(ImGui::GetWindowWidth() - 120.f);
    ImGui::TextColored({ rc.r, rc.g, rc.b, rc.a }, "%s",
        WtoA(ThreatEngine::RiskStr(sel.Level)).c_str());
    ImGui::Separator();

    if (ImGui::BeginTabBar("##dtabs")) {
        if (ImGui::BeginTabItem("Info")) {
            ImGui::Spacing();
            ImVec4 dim = { 0.45f, 0.48f, 0.52f, 1.f };

            ImGui::TextColored(dim, "Full Path");
            ImGui::Indent(10.f);
            ImGui::TextWrapped("%s", WtoA(CleanNtPath(sel.Image)).c_str());
            ImGui::Unindent(10.f);

            ImGui::Spacing();
            ImGui::TextColored(dim, "Parent PID");  ImGui::SameLine(120.f); ImGui::Text("%lu", sel.PPid);
            ImGui::TextColored(dim, "Timestamp");    ImGui::SameLine(120.f); ImGui::TextUnformatted(WtoA(FtToLocal(sel.Ts)).c_str());
            ImGui::TextColored(dim, "Signature");    ImGui::SameLine(120.f); ImGui::Text("%s", WtoA(Cert::Str(sel.Sig)).c_str());
            ImGui::TextColored(dim, "Event Type");   ImGui::SameLine(120.f); ImGui::TextColored(EventTypeColor(sel.EventType), "%s", EventTypeStr(sel.EventType));

            if (sel.UptimeSeconds > 0) {
                LONGLONG h = sel.UptimeSeconds / 3600, m = (sel.UptimeSeconds % 3600) / 60, s = sel.UptimeSeconds % 60;
                ImGui::TextColored(dim, "Uptime");   ImGui::SameLine(120.f);
                ImGui::Text("%lldh %02lldm %02llds", h, m, s);
            }
            if (!sel.Token.IntegrityStr.empty()) {
                ImGui::TextColored(dim, "Integrity"); ImGui::SameLine(120.f);
                ImVec4 tokCol = sel.Token.Elevated ? ImVec4(1.f, 0.4f, 0.3f, 1.f) : ImVec4(0.88f, 0.90f, 0.92f, 1.f);
                ImGui::TextColored(tokCol, "%s%s", WtoA(sel.Token.IntegrityStr).c_str(),
                    sel.Token.Elevated ? " [ELEVATED]" : "");
            }
            if (!sel.Token.UserName.empty()) {
                ImGui::TextColored(dim, "User");     ImGui::SameLine(120.f);
                ImGui::TextUnformatted(WtoA(sel.Token.UserName).c_str());
            }
            if (sel.IsRETool) {
                ImGui::Spacing();
                ImGui::TextColored({1.f, 0.3f, 0.2f, 1.f}, "!! REVERSE ENGINEERING TOOL DETECTED !!");
            }

            if (!sel.Flags.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(dim, "Flags");
                ImGui::Indent(10.f);
                for (const auto& f : sel.Flags)
                    ImGui::BulletText("%s", WtoA(f).c_str());
                ImGui::Unindent(10.f);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            float bw = (ImGui::GetContentRegionAvail().x - 24.f) / 4.f;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.15f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.2f, 0.2f, 1.f));
            if (ImGui::Button("Kill##det", ImVec2(bw, 0))) {
                DWORD killPid = sel.Pid;
                if (KillProcessById(killPid, &app)) {
                    app.EventLog.erase(app.EventLog.begin() + app.SelectedIdx);
                    app.SelectedIdx = -1;
                    app.ShowDetails = false;
                }
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.35f, 0.10f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.45f, 0.15f, 1.f));
            if (ImGui::Button("Suspend##det", ImVec2(bw, 0)))
                SuspendResumeProcess(sel.Pid, true, &app);
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
            if (ImGui::Button("Open Folder##det", ImVec2(bw, 0)))
                OpenFileLocation(sel.Image, &app);
            ImGui::SameLine();
            if (ImGui::Button("Refresh DLLs##det", ImVec2(bw, 0)))
                EnumProcessModules(sel.Pid, app.InspectModules);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Command Line")) {
            ImGui::Spacing();
            if (sel.CmdLine.empty())
                ImGui::TextColored({ 0.45f, 0.48f, 0.52f, 1.f }, "No command line captured.");
            else {
                ImGui::TextWrapped("%s", WtoA(sel.CmdLine).c_str());
                ImGui::Spacing();
                if (ImGui::Button("Copy##cmd"))
                    CopyToClipboard(WtoA(sel.CmdLine));
            }
            ImGui::EndTabItem();
        }

        char modTab[64];
        snprintf(modTab, sizeof(modTab), "Modules (%d)", (int)app.InspectModules.size());
        if (ImGui::BeginTabItem(modTab)) {
            ImGui::Spacing();
            if (app.InspectModules.empty()) {
                ImGui::TextColored({ 0.45f, 0.48f, 0.52f, 1.f },
                    "No modules loaded. Click 'Refresh DLLs' or the process may have exited.");
            }
            else if (ImGui::BeginTable("##mods", 4,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Module",  ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Base",    ImGuiTableColumnFlags_WidthFixed, 120.f);
                ImGui::TableSetupColumn("Size",    ImGuiTableColumnFlags_WidthFixed, 80.f);
                ImGui::TableSetupColumn("Path",    ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& m : app.InspectModules) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(WtoA(m.Name).c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("0x%p", (void*)m.Base);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(FormatBytes(m.Size).c_str());
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(WtoA(m.Path).c_str());
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

static void DrawDriverList(AppState& app)
{
    float dt = ImGui::GetIO().DeltaTime;
    app.DriverRefreshTimer += dt;

    if (!app.DriversLoaded || app.DriverRefreshTimer >= 30.f) {
        Drivers::Enumerate(app.DriverList);
        app.DriversLoaded = true;
        app.DriverRefreshTimer = 0.f;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.f);
    ImGui::InputTextWithHint("##drvfilter", "Filter drivers...",
        app.DriverFilter, sizeof(app.DriverFilter));
    ImGui::SameLine();
    if (ImGui::Button("Refresh##drv", ImVec2(80.f, 0))) {
        Drivers::Enumerate(app.DriverList);
        app.DriverRefreshTimer = 0.f;
        SetStatusMsg(app, "Refreshed %d drivers", (int)app.DriverList.size());
    }
    ImGui::Spacing();

    int total = (int)app.DriverList.size();
    int signedCount = 0, unsignedCount = 0;
    for (const auto& d : app.DriverList) {
        if (d.Signed) signedCount++; else unsignedCount++;
    }
    ImGui::TextColored({ 0.45f, 0.48f, 0.52f, 1.f }, "Total: %d", total);
    ImGui::SameLine();
    ImGui::TextColored({ 0.2f, 0.85f, 0.45f, 1.f }, "Signed: %d", signedCount);
    ImGui::SameLine();
    if (unsignedCount > 0)
        ImGui::TextColored({ 1.f, 0.3f, 0.2f, 1.f }, "UNSIGNED: %d", unsignedCount);
    else
        ImGui::TextColored({ 0.2f, 0.85f, 0.45f, 1.f }, "Unsigned: 0");
    ImGui::Spacing();

    std::string filterLower;
    if (app.DriverFilter[0]) {
        filterLower = app.DriverFilter;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
    }

    if (ImGui::BeginTable("Drivers", 4,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name",    ImGuiTableColumnFlags_WidthFixed, 160.f);
        ImGui::TableSetupColumn("Signed",  ImGuiTableColumnFlags_WidthFixed, 60.f);
        ImGui::TableSetupColumn("Signer",  ImGuiTableColumnFlags_WidthFixed, 180.f);
        ImGui::TableSetupColumn("Path",    ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& d : app.DriverList) {
            if (!filterLower.empty()) {
                std::string nm = WtoA(d.Name);
                std::transform(nm.begin(), nm.end(), nm.begin(), ::tolower);
                std::string pa = WtoA(d.Path);
                std::transform(pa.begin(), pa.end(), pa.begin(), ::tolower);
                std::string sg = WtoA(d.Signer);
                std::transform(sg.begin(), sg.end(), sg.begin(), ::tolower);
                if (nm.find(filterLower) == std::string::npos &&
                    pa.find(filterLower) == std::string::npos &&
                    sg.find(filterLower) == std::string::npos)
                    continue;
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(WtoA(d.Name).c_str());

            ImGui::TableSetColumnIndex(1);
            if (d.Signed)
                ImGui::TextColored({ 0.2f, 0.85f, 0.45f, 1.f }, "YES");
            else
                ImGui::TextColored({ 1.f, 0.3f, 0.2f, 1.f }, "NO");

            ImGui::TableSetColumnIndex(2);
            if (!d.Signer.empty())
                ImGui::TextUnformatted(WtoA(d.Signer).c_str());
            else if (!d.Signed)
                ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1.f }, "-");

            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(WtoA(d.Path).c_str());
            if (ImGui::IsItemHovered() && !d.Path.empty()) {
                ImGui::BeginTooltip();
                ImGui::Text("Base: 0x%p", d.BaseAddress);
                ImGui::Text("Path: %s", WtoA(d.Path).c_str());
                ImGui::EndTooltip();
            }
        }
        ImGui::EndTable();
    }
}

static void DrawSecurity(AppState& app)
{
    ImVec4 dim  = { 0.45f, 0.48f, 0.52f, 1.f };
    ImVec4 acc  = { 0.16f, 0.80f, 0.50f, 1.f };
    ImVec4 warn = { 1.0f, 0.4f, 0.3f, 1.f };
    float dt = ImGui::GetIO().DeltaTime;

    ImGui::TextColored(acc, "SELF-PROTECTION");
    ImGui::Spacing();

    if (!app.SelfProtectEnabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.45f, 0.30f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.60f, 0.38f, 1.f));
        if (ImGui::Button("Enable Kernel Protection", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            if (app.pDriver && app.pDriver->IsOpen()) {
                app.pDriver->SetProtectedPid(GetCurrentProcessId());
                app.SelfProtectEnabled = true;
                SetStatusMsg(app, "Kernel process protection ENABLED for PID %lu", GetCurrentProcessId());
                DbgLog::Write(LogSev::Ok, L"Security",
                    std::format(L"Kernel protection enabled for PID={}", GetCurrentProcessId()));
            } else {
                SetStatusMsg(app, "Cannot enable protection: driver not connected");
            }
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::TextColored(acc, "ACTIVE - PID %lu protected by ObRegisterCallbacks", GetCurrentProcessId());
        ImGui::TextColored(dim, "External processes cannot terminate, write memory, or tamper.");
    }

    ImGui::Spacing();

    if (!app.AntiDbgHidden) {
        if (ImGui::Button("Hide Thread From Debuggers", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            if (AntiDbg::HideFromDebugger()) {
                app.AntiDbgHidden = true;
                SetStatusMsg(app, "Main thread hidden from debuggers");
            } else {
                SetStatusMsg(app, "Failed to hide thread from debuggers");
            }
        }
    } else {
        ImGui::TextColored(acc, "Thread hidden from debuggers");
    }

    ImGui::Spacing();
    {
        bool dbg = AntiDbg::IsBeingDebugged();
        if (dbg)
            ImGui::TextColored(warn, "WARNING: Debugger detected attached to this process!");
        else
            ImGui::TextColored(acc, "No debugger detected");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(acc, "RE TOOL DETECTION");
    ImGui::Spacing();

    app.REToolScanTimer += dt;
    if (app.REToolScanTimer >= 5.f) {
        AntiDbg::ScanForRETools(app.DetectedRETools);
        app.REToolScanTimer = 0.f;
    }

    if (ImGui::Button("Scan Now##re", ImVec2(120.f, 0))) {
        AntiDbg::ScanForRETools(app.DetectedRETools);
        app.REToolScanTimer = 0.f;
        SetStatusMsg(app, "RE scan: found %d tools", (int)app.DetectedRETools.size());
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-block RE tools", &app.REToolAutoBlock);

    ImGui::Spacing();

    if (app.DetectedRETools.empty()) {
        ImGui::TextColored(acc, "No reverse-engineering tools detected");
    } else {
        ImGui::TextColored(warn, "%d RE tool(s) running:", (int)app.DetectedRETools.size());
        ImGui::Spacing();

        if (ImGui::BeginTable("##retools", 3,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
            ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Tool",   ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("PID",    ImGuiTableColumnFlags_WidthFixed, 60.f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80.f);
            ImGui::TableHeadersRow();

            for (auto& rt : app.DetectedRETools) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(warn, "%s", WtoA(rt.Name).c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%lu", rt.Pid);
                ImGui::TableSetColumnIndex(2);
                char killId[32]; snprintf(killId, sizeof(killId), "Kill##re%lu", rt.Pid);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.15f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.2f, 0.2f, 1.f));
                if (ImGui::SmallButton(killId)) {
                    KillProcessById(rt.Pid, &app);
                }
                ImGui::PopStyleColor(2);
            }
            ImGui::EndTable();
        }

        if (app.REToolAutoBlock) {
            for (auto& rt : app.DetectedRETools) {
                KillProcessById(rt.Pid, &app);
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(acc, "PRIVILEGE ESCALATION MONITOR");
    ImGui::Spacing();
    ImGui::TextColored(dim, "Recent elevated processes from Event Log:");
    ImGui::Spacing();

    int elevCount = 0;
    if (ImGui::BeginTable("##elev", 5,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingStretchProp,
        ImVec2(0, 200.f)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("PID",       ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("Image",     ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Elevated",  ImGuiTableColumnFlags_WidthFixed, 60.f);
        ImGui::TableSetupColumn("Integrity", ImGuiTableColumnFlags_WidthFixed, 80.f);
        ImGui::TableSetupColumn("User",      ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (int i = (int)app.EventLog.size() - 1; i >= 0 && elevCount < 50; i--) {
            auto& r = app.EventLog[i];
            if (r.EventType != EventProcessCreate) continue;
            if (!r.Token.Elevated && r.Token.IntegrityLevel < SECURITY_MANDATORY_HIGH_RID)
                continue;

            elevCount++;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%lu", r.Pid);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(FileName(r.Image).c_str());
            ImGui::TableSetColumnIndex(2);
            if (r.Token.Elevated)
                ImGui::TextColored(warn, "YES");
            else
                ImGui::TextColored(dim, "no");
            ImGui::TableSetColumnIndex(3);
            if (r.Token.IntegrityLevel >= SECURITY_MANDATORY_SYSTEM_RID)
                ImGui::TextColored(warn, "%s", WtoA(r.Token.IntegrityStr).c_str());
            else if (r.Token.IntegrityLevel >= SECURITY_MANDATORY_HIGH_RID)
                ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, "%s", WtoA(r.Token.IntegrityStr).c_str());
            else
                ImGui::TextUnformatted(WtoA(r.Token.IntegrityStr).c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(WtoA(r.Token.UserName).c_str());
        }
        ImGui::EndTable();
    }

    if (elevCount == 0)
        ImGui::TextColored(dim, "No elevated processes detected yet.");
}

bool RenderInit(HWND hwnd, DxState* dx)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename  = "kmprocmon_imgui.ini";

    ApplyModernTheme();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dx->pDevice, dx->pContext);

    return true;
}

void RenderShutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

static bool s_Dragging = false;
static POINT s_DragStart{};

void RenderFrame(DxState* dx, AppState& app)
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##main", nullptr, wf);

    // title bar
    float barH = 36.f;
    float winW = ImGui::GetWindowWidth();
    ImVec2 barMin = ImGui::GetCursorScreenPos();
    ImVec2 barMax = { barMin.x + winW, barMin.y + barH };

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(barMin, barMax, IM_COL32(14, 14, 18, 255));
    dl->AddRectFilled(
        { barMin.x, barMax.y - 2.f }, barMax,
        IM_COL32(30, 190, 100, 100));

    ImGui::SetCursorScreenPos({ barMin.x + 14.f, barMin.y + 8.f });
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.16f, 0.80f, 0.50f, 1.f));
    ImGui::Text("KERNEL PROCESS MONITOR");
    ImGui::PopStyleColor();

    bool connected = app.pDriver && app.pDriver->IsOpen();
    float statusX = barMin.x + winW - 250.f;
    ImGui::SetCursorScreenPos({ statusX, barMin.y + 8.f });

    dl->AddCircleFilled(
        { statusX - 8.f, barMin.y + barH * 0.5f }, 4.f,
        connected ? IM_COL32(40, 200, 110, 255) : IM_COL32(220, 60, 60, 255));

    if (connected) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.85f, 0.45f, 1.f));
        ImGui::Text("DRIVER ONLINE");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.30f, 0.30f, 1.f));
        ImGui::Text("OFFLINE");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.30f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.42f, 1.f));
        if (ImGui::SmallButton("Retry")) {
            if (app.pDriver) app.pDriver->Open();
        }
        ImGui::PopStyleColor(2);
    }

    float btnW = 32.f;
    float btnY = barMin.y + 5.f;
    float btnH = 26.f;

    ImGui::SetCursorScreenPos({ barMin.x + winW - btnW * 2 - 12.f, btnY });
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.38f, 1.f));
    if (ImGui::Button("_##min", ImVec2(btnW, btnH)))
        ShowWindow(app.hWnd, SW_MINIMIZE);
    ImGui::PopStyleColor(2);

    ImGui::SameLine(0, 4.f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.18f, 0.18f, 1.f));
    if (ImGui::Button("X##cls", ImVec2(btnW, btnH)))
        app.Running = false;
    ImGui::PopStyleColor(2);

    ImVec2 mpos = ImGui::GetMousePos();
    bool onBar = mpos.x >= barMin.x && mpos.x <= barMax.x - btnW * 2 - 20.f
              && mpos.y >= barMin.y && mpos.y <= barMax.y;

    if (onBar && ImGui::IsMouseClicked(0)) {
        s_Dragging = true;
        POINT pt; GetCursorPos(&pt);
        s_DragStart = pt;
    }
    if (s_Dragging) {
        if (ImGui::IsMouseDown(0)) {
            POINT pt; GetCursorPos(&pt);
            RECT wr; GetWindowRect(app.hWnd, &wr);
            int dx2 = pt.x - s_DragStart.x;
            int dy2 = pt.y - s_DragStart.y;
            SetWindowPos(app.hWnd, nullptr,
                wr.left + dx2, wr.top + dy2, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            s_DragStart = pt;
        } else {
            s_Dragging = false;
        }
    }

    ImGui::SetCursorScreenPos({ barMin.x, barMax.y });
    ImGui::PopStyleVar();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

    float statusBarH = 24.f;
    float contentH = ImGui::GetContentRegionAvail().y - statusBarH - 4.f;

    float leftW = 280.f;
    ImGui::BeginChild("##left", ImVec2(leftW, contentH), true);

    if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen))
        DrawStats(app);

    if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen))
        DrawConfig(app);

    if (ImGui::CollapsingHeader("Stealth / Cloak"))
        DrawCloak(app);

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##right", ImVec2(0, contentH), true);

    if (ImGui::BeginTabBar("##mainTabs")) {

        char evtLabel[64];
        snprintf(evtLabel, sizeof(evtLabel), "Events (%d)###evttab", (int)app.EventLog.size());
        if (ImGui::BeginTabItem(evtLabel)) {
            app.MainTab = 0;
            ImGui::Spacing();
            DrawEventLog(app);
            ImGui::EndTabItem();
        }

        char drvLabel[64];
        snprintf(drvLabel, sizeof(drvLabel), "Drivers (%d)###drvtab", (int)app.DriverList.size());
        if (ImGui::BeginTabItem(drvLabel)) {
            app.MainTab = 1;
            ImGui::Spacing();
            DrawDriverList(app);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Security###sectab")) {
            app.MainTab = 2;
            ImGui::Spacing();
            DrawSecurity(app);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::EndChild();

    {
        float dt = ImGui::GetIO().DeltaTime;
        if (app.StatusTimer > 0.f) app.StatusTimer -= dt;

        ImVec2 sbMin = ImGui::GetCursorScreenPos();
        float sbW = ImGui::GetWindowWidth();
        ImVec2 sbMax = { sbMin.x + sbW, sbMin.y + statusBarH };
        ImDrawList* sdl = ImGui::GetWindowDrawList();
        sdl->AddRectFilled(sbMin, sbMax, IM_COL32(10, 10, 14, 255));
        sdl->AddLine(sbMin, { sbMax.x, sbMin.y }, IM_COL32(30, 190, 100, 60));

        ImGui::SetCursorScreenPos({ sbMin.x + 10.f, sbMin.y + 4.f });
        if (app.StatusTimer > 0.f && app.StatusMsg[0]) {
            float alpha = (app.StatusTimer < 1.f) ? app.StatusTimer : 1.f;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 0.5f, alpha));
            ImGui::TextUnformatted(app.StatusMsg);
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.37f, 0.42f, 1.f));
            ImGui::Text("Ready  |  Events: %d  |  Drivers: %d  |  Poll: %.1fs",
                (int)app.EventLog.size(), (int)app.DriverList.size(), app.PollInterval);
            ImGui::PopStyleColor();
        }
    }

    ImGui::PopStyleVar();

    ImGui::End();

    DrawProcessDetails(app);

    ImGui::Render();

    float clear[4] = { 0.05f, 0.05f, 0.07f, 1.00f };
    dx->pContext->OMSetRenderTargets(1, &dx->pRenderTarget, nullptr);
    dx->pContext->ClearRenderTargetView(dx->pRenderTarget, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    DxPresent(dx);
}
