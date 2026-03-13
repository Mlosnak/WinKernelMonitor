/*++

Module Name:

    Kernel Process Monitor.cpp

Abstract:

    Entry point. Creates the DirectX 11 overlay window, initializes
    ImGui, opens the driver device (assumed running via sc start),
    and runs the render + poll loop.

--*/

#include <Windows.h>
#include <vector>
#include <format>
#include <chrono>

#include "Logs/dbglog.h"
#include "Usermodecomm/kmio.h"
#include "Functions/threat.h"
#include "Overlay/dx.h"
#include "Overlay/overlay.h"
#include "Overlay/render.h"
#include "Functions/cloak.h"
#include "Functions/antidbg.h"

#pragma comment(lib, "Shell32.lib")

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_     LPWSTR,
    _In_     int)
{
    DbgLog::Init(L"KmProcMon.log", LogSev::Ok);
    DbgLog::Write(LogSev::Ok, L"Kernel Process Monitor starting");

    {
        HANDLE hTest = CreateFileW(
            KMPROCMON_WIN32_DEVICE,
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hTest == INVALID_HANDLE_VALUE) {
            MessageBoxW(nullptr,
                L"Driver not loaded.\n\n"
                L"Run as Administrator:\n"
                L"  sc create KmProcMon type= kernel binPath= \"<path>\\KernelMonitorDriver.sys\"\n"
                L"  sc start KmProcMon",
                L"Kernel Process Monitor",
                MB_ICONERROR | MB_OK);
            DbgLog::Write(LogSev::Fatal, L"Driver device not available, aborting");
            DbgLog::Shutdown();
            return 1;
        }
        CloseHandle(hTest);
    }

    HWND hwnd = OverlayCreate(hInstance, 1280, 780, L"Kernel Process Monitor");
    if (!hwnd) {
        DbgLog::Write(LogSev::Fatal, L"Failed to create window");
        return 1;
    }

    DxState dx{};
    if (!DxCreate(hwnd, &dx)) {
        DbgLog::Write(LogSev::Fatal, L"DirectX 11 initialization failed");
        OverlayDestroy(hwnd);
        return 1;
    }

    if (!RenderInit(hwnd, &dx)) {
        DbgLog::Write(LogSev::Fatal, L"ImGui initialization failed");
        DxDestroy(&dx);
        OverlayDestroy(hwnd);
        return 1;
    }

    KmIo driver;
    ThreatEngine engine;

    AppState app{};
    app.pDriver = &driver;
    app.pEngine = &engine;
    app.hWnd    = hwnd;

    app.Cfg.MonitorThreads     = FALSE;
    app.Cfg.CaptureCommandLine = TRUE;
    app.CaptureCmd             = true;

    if (!driver.Open()) {
        DbgLog::Write(LogSev::Warn, L"Driver device not available. Use: sc start KmProcMon");
    }
    else {
        driver.PushConfig(app.Cfg);
        DbgLog::Write(LogSev::Ok, L"Driver connected, config pushed");

        if (driver.SetProtectedPid(GetCurrentProcessId())) {
            app.SelfProtectEnabled = true;
            DbgLog::Write(LogSev::Ok, L"Security",
                std::format(L"Kernel protection enabled for PID={}", GetCurrentProcessId()));
        }
    }

    auto lastTick = std::chrono::steady_clock::now();

    while (app.Running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) app.Running = false;
        }
        if (!app.Running) break;

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTick).count();
        lastTick = now;

        RECT rc{};
        GetClientRect(hwnd, &rc);
        UINT w = static_cast<UINT>(rc.right - rc.left);
        UINT h = static_cast<UINT>(rc.bottom - rc.top);
        if (w != dx.Width || h != dx.Height)
            DxResize(&dx, w, h);

        app.PollAccum += dt;
        if (app.PollAccum >= app.PollInterval && driver.IsOpen()) {
            app.PollAccum = 0.f;

            std::vector<KMPROCMON_EVENT> events;
            ULONG dropped = 0;
            if (driver.PollEvents(events, dropped)) {
                for (const auto& ev : events) {
                    if (ev.Type == EventProcessCreate || ev.Type == EventProcessExit) {
                        ProcReport rpt = engine.Evaluate(ev);
                        app.EventLog.push_back(std::move(rpt));

                        while ((int)app.EventLog.size() > app.MaxLogEntries)
                            app.EventLog.pop_front();
                    }
                }
            }
        }

        app.StatsAccum += dt;
        if (app.StatsAccum >= 10.f && driver.IsOpen()) {
            app.StatsAccum = 0.f;
            driver.QueryStats(app.Stats);
        }

        RenderFrame(&dx, app);
    }

    DbgLog::Write(LogSev::Ok, L"Shutting down");

    driver.Close();
    RenderShutdown();
    DxDestroy(&dx);
    OverlayDestroy(hwnd);
    DbgLog::Shutdown();

    return 0;
}
