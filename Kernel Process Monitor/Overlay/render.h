#pragma once

#include "dx.h"
#include "../Usermodecomm/kmio.h"
#include "../Functions/threat.h"
#include "../Functions/cloak.h"
#include "../Functions/drivers.h"
#include "../Functions/antidbg.h"
#include <vector>
#include <deque>

struct AppState {
    KmIo*                           pDriver     = nullptr;
    ThreatEngine*                   pEngine     = nullptr;
    HWND                            hWnd        = nullptr;

    std::deque<ProcReport>          EventLog;
    KMPROCMON_STATS                 Stats{};
    KMPROCMON_CONFIG                Cfg{};

    bool                            Running         = true;
    bool                            MonitorThreads  = false;
    bool                            DeepScan        = false;
    bool                            CaptureCmd      = true;

    bool                            CloakCapture    = false;
    bool                            CloakTaskbar    = false;
    bool                            CloakTopmost    = false;
    BYTE                            CloakAlpha      = 255;
    char                            SpoofName[128]  = {};

    int                             MaxLogEntries   = 500;
    float                           PollInterval    = 0.5f;
    float                           PollAccum       = 0.f;
    float                           StatsAccum      = 0.f;

    int                             SelectedIdx     = -1;
    bool                            ShowDetails     = false;
    char                            FilterText[128] = {};
    std::vector<PeModule>           InspectModules;

    bool                            RandomSpoofMode = false;
    float                           RandomSpoofTimer = 0.f;
    float                           RandomSpoofInterval = 0.3f;

    bool                            AutoScroll      = true;
    char                            StatusMsg[256]  = {};
    float                           StatusTimer     = 0.f;

    int                             MainTab         = 0;
    std::vector<DriverInfo>         DriverList;
    float                           DriverRefreshTimer = 0.f;
    bool                            DriversLoaded   = false;
    char                            DriverFilter[128] = {};

    bool                            SelfProtectEnabled = false;
    bool                            AntiDbgHidden   = false;
    std::vector<ReToolInfo>         DetectedRETools;
    float                           REToolScanTimer = 0.f;
    bool                            REToolAutoBlock = false;
};

bool    RenderInit(HWND hwnd, DxState* dx);
void    RenderShutdown();
void    RenderFrame(DxState* dx, AppState& app);
