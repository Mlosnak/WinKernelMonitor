# Kernel Process Monitor

Real-time process monitor for Windows built on a **kernel driver** (Ring 0) + **user-mode UI** (Ring 3) architecture. The driver hooks process/thread creation via kernel callbacks and streams telemetry to a DirectX 11 / ImGui dashboard that performs risk analysis, signature verification, and provides stealth controls.

<img width="1269" height="771" alt="kernel1" src="https://github.com/user-attachments/assets/f047d35a-d7b6-4ed3-ab71-5612d497d82b" />
<img width="1270" height="773" alt="Kernel3" src="https://github.com/user-attachments/assets/3edb4527-a13e-44d4-9370-82c7981a20bc" />
<img width="1264" height="766" alt="Kernel2" src="https://github.com/user-attachments/assets/a4fe81bd-46d2-451a-940f-f7e75217d51b" />


## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                   User-Mode (Ring 3)                      │
│                                                          │
│  ┌──────────┐  ┌──────────┐  ┌────────┐  ┌───────────┐ │
│  │ dx       │  │ overlay  │  │ render │  │ cloak     │ │
│  │ DX11 GPU │  │ Win32 Wnd│  │ ImGui  │  │ Stealth   │ │
│  └──────────┘  └──────────┘  └────────┘  └───────────┘ │
│  ┌──────────┐  ┌──────────┐  ┌────────┐  ┌───────────┐ │
│  │ kmio     │  │ threat   │  │ cert   │  │ antidbg   │ │
│  │ IOCTL I/O│  │ Risk Eng │  │ Trust  │  │ RE Detect │ │
│  └─────┬────┘  └──────────┘  └────────┘  └───────────┘ │
│        │       ┌──────────┐  ┌────────┐                 │
│        │       │ dbglog   │  │ pe     │                 │
│        │       │ Log+ODS  │  │ Modules│                 │
│        │       └──────────┘  └────────┘                 │
└────────┼────────────────────────────────────────────────┘
         │ DeviceIoControl  \\.\KmProcMon
┌────────┼────────────────────────────────────────────────┐
│        ▼             Kernel-Mode (Ring 0)                │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐ │
│  │ Queue.c      │  │ ProcessMon.c │  │ ObProtect.c   │ │
│  │ IOCTL Disp.  │  │ Ps Notify    │  │ Anti-Tamper   │ │
│  └──────────────┘  └──────────────┘  └───────────────┘ │
│  ┌──────────────┐  ┌──────────────┐                    │
│  │ Device.c     │  │ Driver.c     │                    │
│  │ Ring Buffer  │  │ Entry + Init │                    │
│  └──────────────┘  └──────────────┘                    │
└─────────────────────────────────────────────────────────┘
```

## Features

**Kernel driver** hooks into `PsSetCreateProcessNotifyRoutineEx` and `PsSetCreateThreadNotifyRoutine` to capture process/thread lifecycle events with image path, command line, and timestamps. Events are queued in a spinlock-protected ring buffer and drained by user-mode via `METHOD_BUFFERED` IOCTLs. `ObRegisterCallbacks` strips `PROCESS_TERMINATE` and `PROCESS_VM_WRITE` from handles targeting the monitor, preventing external tampering.

**User-mode app** polls the driver, runs each event through a risk engine that checks signature (`WinVerifyTrust` + catalog fallback), execution directory, and known RE tool names. Results appear in a tabbed ImGui dashboard:

| Tab | What it does |
|---|---|
| **Events** | Scrollable log with time, PID, PPID, risk level, image, flags. Right-click to kill/suspend/inspect DLLs. |
| **Drivers** | Lists all loaded kernel drivers with signer and signature status. Filter + refresh. |
| **Security** | Kernel self-protection toggle, debugger detection, RE tool scanner with auto-block, privilege escalation monitor. |

Sidebar panels: **Statistics** (counters, uptime), **Configuration** (toggle thread mon, cmd capture, poll rate), **Stealth** (exclude from capture, taskbar hide, transparency, PEB name spoofing).

Risk levels: `CLEAN` → `LOW` → `MEDIUM` → `HIGH` → `CRITICAL`, each color-coded.

## IOCTL Interface

| Code | Direction | Description |
|---|---|---|
| `0x800` | Read | Drain events from ring buffer |
| `0x801` | Write | Clear event buffer |
| `0x802` | Read | Query driver statistics (counters + uptime) |
| `0x803` | Write | Push runtime config (thread monitoring, cmd capture) |
| `0x804` | Write | Set protected PID for ObRegisterCallbacks |

All IOCTLs use `METHOD_BUFFERED` through device `\\.\KmProcMon`.

## Building

**Requirements:** Visual Studio 2022+, WDK 10.0+, Windows SDK 10.0+, C++20.

1. Open `Kernel Process Monitor.slnx` in Visual Studio
2. Set platform to **x64**
3. Build **Kernel Monitor Driver** → `KernelMonitorDriver.sys`
4. Build **Kernel Process Monitor** → `Kernel Process Monitor.exe`

## Usage

Load the driver (admin cmd):
```
sc create KmProcMon type= kernel binPath= "C:\path\to\KernelMonitorDriver.sys"
sc start KmProcMon
```

Run `Kernel Process Monitor.exe` — it connects to `\\.\KmProcMon` automatically.

Unload:
```
sc stop KmProcMon
sc delete KmProcMon
```

## Notes

- Driver requires **test signing** (`bcdedit /set testsigning on`) unless properly signed
- Run the app as **Administrator**
- Diagnostic output goes to `KmProcMon.log` and `OutputDebugString` (use DbgView)
- Enable "Capture Kernel" in DbgView for driver-side `[KmProcMon]` messages
- The driver is purely observational — it does not block or deny process creation

## License

See LICENSE file for details.
