/*++

Module Name:

    KmProcMonShared.h

Abstract:

    Shared definitions between the kernel-mode driver and user-mode application.
    Contains IOCTL codes, event structures, and communication protocol constants.

Environment:

    Kernel-mode and User-mode

--*/

#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include <Windows.h>
#endif

#define KMPROCMON_DEVICE_NAME       L"\\Device\\KmProcMon"
#define KMPROCMON_SYMBOLIC_LINK     L"\\DosDevices\\KmProcMon"
#define KMPROCMON_WIN32_DEVICE      L"\\\\.\\KmProcMon"
#define KMPROCMON_SERVICE_NAME      L"KmProcMon"
#define KMPROCMON_DISPLAY_NAME      L"Kernel Process Monitor Driver"
#define KMPROCMON_DRIVER_FILENAME   L"KernelMonitorDriver.sys"
#define KMPROCMON_ALTITUDE          L"385200"

#define KMPROCMON_MAX_PATH          520
#define KMPROCMON_EVENT_BUFFER_SIZE 256

//
// IOCTL definitions using METHOD_BUFFERED for safety
//
#define KMPROCMON_DEVICE_TYPE       0x8000

#define IOCTL_KMPROCMON_GET_EVENTS \
    CTL_CODE(KMPROCMON_DEVICE_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_KMPROCMON_CLEAR_EVENTS \
    CTL_CODE(KMPROCMON_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_KMPROCMON_GET_STATS \
    CTL_CODE(KMPROCMON_DEVICE_TYPE, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_KMPROCMON_SET_CONFIG \
    CTL_CODE(KMPROCMON_DEVICE_TYPE, 0x803, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_KMPROCMON_SET_PROTECTED_PID \
    CTL_CODE(KMPROCMON_DEVICE_TYPE, 0x804, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// Event types tracked by the driver
//
typedef enum _KMPROCMON_EVENT_TYPE {
    EventProcessCreate  = 0,
    EventProcessExit    = 1,
    EventThreadCreate   = 2,
    EventThreadExit     = 3
} KMPROCMON_EVENT_TYPE;

//
// Single telemetry event captured in Ring 0
//
#pragma pack(push, 8)
typedef struct _KMPROCMON_EVENT {
    KMPROCMON_EVENT_TYPE    Type;
    ULONG                   ProcessId;
    ULONG                   ParentProcessId;
    ULONG                   ThreadId;
    LARGE_INTEGER           Timestamp;
    NTSTATUS                ExitStatus;
    WCHAR                   ImagePath[KMPROCMON_MAX_PATH];
    WCHAR                   CommandLine[KMPROCMON_MAX_PATH];
} KMPROCMON_EVENT, *PKMPROCMON_EVENT;
#pragma pack(pop)

//
// Batch response header for IOCTL_KMPROCMON_GET_EVENTS
//
typedef struct _KMPROCMON_EVENT_BATCH {
    ULONG               Count;
    ULONG               Dropped;
    KMPROCMON_EVENT      Events[1]; // variable-length array
} KMPROCMON_EVENT_BATCH, *PKMPROCMON_EVENT_BATCH;

//
// Driver statistics
//
typedef struct _KMPROCMON_STATS {
    LONGLONG    TotalProcessCreated;
    LONGLONG    TotalProcessExited;
    LONGLONG    TotalThreadCreated;
    LONGLONG    TotalThreadExited;
    LONGLONG    EventsDropped;
    LONGLONG    UptimeTicks;
} KMPROCMON_STATS, *PKMPROCMON_STATS;

//
// Runtime configuration sent from user-mode
//
typedef struct _KMPROCMON_CONFIG {
    BOOLEAN     MonitorThreads;
    BOOLEAN     CaptureCommandLine;
} KMPROCMON_CONFIG, *PKMPROCMON_CONFIG;
