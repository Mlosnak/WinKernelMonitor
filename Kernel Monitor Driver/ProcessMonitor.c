/*++

Module Name:

    ProcessMonitor.c

Abstract:

    Implements the process and thread notification callbacks.
    Captures telemetry (PID, parent PID, image path, command line,
    timestamps) and pushes it into the device context ring buffer.

Environment:

    Kernel-mode (PASSIVE_LEVEL for process, PASSIVE_LEVEL for thread)

--*/

#include "driver.h"

static VOID
SafeCopyUnicodeString(
    _Out_writes_(MaxChars) PWCHAR  Dest,
    _In_  PCUNICODE_STRING         Source,
    _In_  ULONG                    MaxChars
    )
{
    ULONG charsToCopy;

    if (!Source || !Source->Buffer || Source->Length == 0) {
        Dest[0] = L'\0';
        return;
    }

    charsToCopy = Source->Length / sizeof(WCHAR);
    if (charsToCopy >= MaxChars) {
        charsToCopy = MaxChars - 1;
    }

    RtlCopyMemory(Dest, Source->Buffer, charsToCopy * sizeof(WCHAR));
    Dest[charsToCopy] = L'\0';
}

VOID
ProcessNotifyCallback(
    _Inout_  PEPROCESS              Process,
    _In_     HANDLE                 ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
    )
{
    PKMPROCMON_EVENT    pEv;
    PDEVICE_CONTEXT     ctx = g_DeviceContext;

    UNREFERENCED_PARAMETER(Process);

    if (!ctx) return;

    //
    // Allocate event from pool - too large for kernel stack (~2KB)
    //
    pEv = (PKMPROCMON_EVENT)ExAllocatePool2(
        POOL_FLAG_NON_PAGED, sizeof(KMPROCMON_EVENT), 'vEpK');
    if (!pEv) return;

    RtlZeroMemory(pEv, sizeof(*pEv));
    KeQuerySystemTimePrecise(&pEv->Timestamp);
    pEv->ProcessId = HandleToULong(ProcessId);

    if (CreateInfo) {
        //
        // Process creation
        //
        pEv->Type           = EventProcessCreate;
        pEv->ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);

        KmDbg("PROC+ PID=%lu PPID=%lu", pEv->ProcessId, pEv->ParentProcessId);

        if (CreateInfo->ImageFileName) {
            SafeCopyUnicodeString(
                pEv->ImagePath,
                CreateInfo->ImageFileName,
                KMPROCMON_MAX_PATH
                );
        }

        if (ctx->Config.CaptureCommandLine && CreateInfo->CommandLine) {
            SafeCopyUnicodeString(
                pEv->CommandLine,
                CreateInfo->CommandLine,
                KMPROCMON_MAX_PATH
                );
        }

        InterlockedIncrement64(&ctx->TotalProcessCreated);
    }
    else {
        //
        // Process exit
        //
        pEv->Type = EventProcessExit;
        InterlockedIncrement64(&ctx->TotalProcessExited);
        KmDbgTrace("PROC- PID=%lu", pEv->ProcessId);
    }

    DeviceContextPushEvent(ctx, pEv);
    ExFreePoolWithTag(pEv, 'vEpK');
}

VOID
ThreadNotifyCallback(
    _In_ HANDLE  ProcessId,
    _In_ HANDLE  ThreadId,
    _In_ BOOLEAN Create
    )
{
    PKMPROCMON_EVENT    pEv;
    PDEVICE_CONTEXT     ctx = g_DeviceContext;

    if (!ctx) return;

    if (!ctx->Config.MonitorThreads) return;

    pEv = (PKMPROCMON_EVENT)ExAllocatePool2(
        POOL_FLAG_NON_PAGED, sizeof(KMPROCMON_EVENT), 'vEpK');
    if (!pEv) return;

    RtlZeroMemory(pEv, sizeof(*pEv));
    KeQuerySystemTimePrecise(&pEv->Timestamp);

    pEv->ProcessId = HandleToULong(ProcessId);
    pEv->ThreadId  = HandleToULong(ThreadId);

    if (Create) {
        pEv->Type = EventThreadCreate;
        InterlockedIncrement64(&ctx->TotalThreadCreated);
        KmDbgTrace("THRD+ PID=%lu TID=%lu", pEv->ProcessId, pEv->ThreadId);
    }
    else {
        pEv->Type = EventThreadExit;
        InterlockedIncrement64(&ctx->TotalThreadExited);
    }

    DeviceContextPushEvent(ctx, pEv);
    ExFreePoolWithTag(pEv, 'vEpK');
}
