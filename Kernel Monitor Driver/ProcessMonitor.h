/*++

Module Name:

    ProcessMonitor.h

Abstract:

    Declarations for PsSetCreateProcessNotifyRoutineEx and
    PsSetCreateThreadNotifyRoutine callback handlers.

Environment:

    Kernel-mode

--*/

#pragma once

EXTERN_C_START

VOID
ProcessNotifyCallback(
    _Inout_  PEPROCESS              Process,
    _In_     HANDLE                 ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
    );

VOID
ThreadNotifyCallback(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
    );

EXTERN_C_END
