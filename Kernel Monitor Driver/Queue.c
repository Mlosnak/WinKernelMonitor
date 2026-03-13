/*++

Module Name:

    queue.c

Abstract:

    IOCTL dispatch for the kernel process monitor. Handles event
    retrieval, buffer clear, statistics query, and config updates.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, KernelMonitorDriverQueueInitialize)
#endif

NTSTATUS
KernelMonitorDriverQueueInitialize(
    _In_ WDFDEVICE Device
    )
{
    WDFQUEUE            queue;
    NTSTATUS            status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoDeviceControl  = KernelMonitorDriverEvtIoDeviceControl;
    queueConfig.EvtIoStop           = KernelMonitorDriverEvtIoStop;

    status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
            "WdfIoQueueCreate failed: 0x%08X", status);
    }

    return status;
}

//
// Drain up to N events from the ring buffer into the user output buffer.
//
static ULONG
DrainEvents(
    _In_  PDEVICE_CONTEXT   Ctx,
    _Out_ PKMPROCMON_EVENT  OutEvents,
    _In_  ULONG             MaxEvents
    )
{
    KIRQL   oldIrql;
    ULONG   copied = 0;

    KeAcquireSpinLock(&Ctx->BufferLock, &oldIrql);

    while (copied < MaxEvents && Ctx->Count > 0) {
        RtlCopyMemory(
            &OutEvents[copied],
            &Ctx->EventBuffer[Ctx->Tail],
            sizeof(KMPROCMON_EVENT)
            );
        Ctx->Tail = (Ctx->Tail + 1) % KMPROCMON_EVENT_BUFFER_SIZE;
        InterlockedDecrement(&Ctx->Count);
        copied++;
    }

    KeReleaseSpinLock(&Ctx->BufferLock, oldIrql);
    return copied;
}

VOID
KernelMonitorDriverEvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
    )
{
    NTSTATUS        status = STATUS_SUCCESS;
    PDEVICE_CONTEXT ctx;
    size_t          bytesReturned = 0;
    PVOID           outBuf = NULL;
    PVOID           inBuf  = NULL;
    WDFDEVICE       device;

    UNREFERENCED_PARAMETER(InputBufferLength);

    device = WdfIoQueueGetDevice(Queue);
    ctx    = DeviceGetContext(device);

    KmDbgTrace("IOCTL dispatch: code=0x%08X outLen=%llu inLen=%llu", IoControlCode, (ULONGLONG)OutputBufferLength, (ULONGLONG)InputBufferLength);

    switch (IoControlCode) {

    case IOCTL_KMPROCMON_GET_EVENTS:
    {
        ULONG maxEvents;
        ULONG headerSize;
        PKMPROCMON_EVENT_BATCH batch;

        headerSize = FIELD_OFFSET(KMPROCMON_EVENT_BATCH, Events);

        if (OutputBufferLength < headerSize + sizeof(KMPROCMON_EVENT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(Request, headerSize, &outBuf, NULL);
        if (!NT_SUCCESS(status)) break;

        batch     = (PKMPROCMON_EVENT_BATCH)outBuf;
        maxEvents = (ULONG)((OutputBufferLength - headerSize) / sizeof(KMPROCMON_EVENT));

        batch->Count   = DrainEvents(ctx, batch->Events, maxEvents);
        batch->Dropped = (ULONG)ctx->EventsDropped;

        if (batch->Count > 0) {
            KmDbg("GET_EVENTS: drained %lu events, %lu dropped total", batch->Count, batch->Dropped);
        }

        bytesReturned = headerSize + (batch->Count * sizeof(KMPROCMON_EVENT));
        break;
    }

    case IOCTL_KMPROCMON_CLEAR_EVENTS:
    {
        KIRQL oldIrql;
        KeAcquireSpinLock(&ctx->BufferLock, &oldIrql);
        ctx->Head  = 0;
        ctx->Tail  = 0;
        ctx->Count = 0;
        KeReleaseSpinLock(&ctx->BufferLock, oldIrql);
        break;
    }

    case IOCTL_KMPROCMON_GET_STATS:
    {
        PKMPROCMON_STATS stats;
        LARGE_INTEGER    now;

        if (OutputBufferLength < sizeof(KMPROCMON_STATS)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(
            Request, sizeof(KMPROCMON_STATS), &outBuf, NULL);
        if (!NT_SUCCESS(status)) break;

        stats = (PKMPROCMON_STATS)outBuf;
        stats->TotalProcessCreated = ctx->TotalProcessCreated;
        stats->TotalProcessExited  = ctx->TotalProcessExited;
        stats->TotalThreadCreated  = ctx->TotalThreadCreated;
        stats->TotalThreadExited   = ctx->TotalThreadExited;
        stats->EventsDropped       = ctx->EventsDropped;

        KeQuerySystemTimePrecise(&now);
        stats->UptimeTicks = now.QuadPart - ctx->StartTime.QuadPart;

        bytesReturned = sizeof(KMPROCMON_STATS);
        break;
    }

    case IOCTL_KMPROCMON_SET_CONFIG:
    {
        PKMPROCMON_CONFIG cfg;

        if (InputBufferLength < sizeof(KMPROCMON_CONFIG)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputBuffer(
            Request, sizeof(KMPROCMON_CONFIG), &inBuf, NULL);
        if (!NT_SUCCESS(status)) break;

        cfg = (PKMPROCMON_CONFIG)inBuf;
        ctx->Config.MonitorThreads     = cfg->MonitorThreads;
        ctx->Config.CaptureCommandLine = cfg->CaptureCommandLine;
        KmDbg("SET_CONFIG: threads=%d cmdline=%d", cfg->MonitorThreads, cfg->CaptureCommandLine);
        break;
    }

    case IOCTL_KMPROCMON_SET_PROTECTED_PID:
    {
        ULONG* pPid;

        if (InputBufferLength < sizeof(ULONG)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputBuffer(
            Request, sizeof(ULONG), &inBuf, NULL);
        if (!NT_SUCCESS(status)) break;

        pPid = (ULONG*)inBuf;
        ObProtectSetPid((LONG)*pPid);
        KmDbg("SET_PROTECTED_PID: pid=%lu", *pPid);
        break;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}

VOID
KernelMonitorDriverEvtIoStop(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG      ActionFlags
    )
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(ActionFlags);

    WdfRequestStopAcknowledge(Request, TRUE);
}
