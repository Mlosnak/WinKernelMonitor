/*++

Module Name:

    device.c

Abstract:

    Creates the control device object (\Device\KmProcMon) and its
    symbolic link, initializes the event ring buffer and I/O queue.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"

PDEVICE_CONTEXT g_DeviceContext = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, KernelMonitorDriverCreateDevice)
#endif

NTSTATUS
KernelMonitorDriverCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT         ctx;
    WDFDEVICE               device;
    NTSTATUS                status;
    DECLARE_CONST_UNICODE_STRING(deviceName, KMPROCMON_DEVICE_NAME);
    DECLARE_CONST_UNICODE_STRING(symLink,    KMPROCMON_SYMBOLIC_LINK);

    PAGED_CODE();

    //
    // We are a software-only (control) device, not a PnP device.
    //
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetCharacteristics(DeviceInit, FILE_DEVICE_SECURE_OPEN, FALSE);
    WdfDeviceInitAssignName(DeviceInit, &deviceName);

    //
    // Allow non-admin reads so the monitor app can work with medium IL
    // while requiring write for config changes.
    //
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    //
    // WdfDeviceCreate consumes DeviceInit regardless of success/failure.
    // After this call, DeviceInit is NULL and must never be freed.
    //
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        KmDbgError("WdfDeviceCreate failed: 0x%08X", status);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "WdfDeviceCreate failed: 0x%08X", status);
        return status;
    }

    ctx = DeviceGetContext(device);
    RtlZeroMemory(ctx, sizeof(DEVICE_CONTEXT));
    KeInitializeSpinLock(&ctx->BufferLock);
    KeQuerySystemTimePrecise(&ctx->StartTime);

    ctx->Config.MonitorThreads      = TRUE;
    ctx->Config.CaptureCommandLine  = TRUE;

    KmDbg("Device context initialized, ring buffer capacity=%d", KMPROCMON_EVENT_BUFFER_SIZE);

    status = WdfDeviceCreateSymbolicLink(device, &symLink);
    if (!NT_SUCCESS(status)) {
        KmDbgError("WdfDeviceCreateSymbolicLink failed: 0x%08X", status);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "WdfDeviceCreateSymbolicLink failed: 0x%08X", status);
        WdfObjectDelete(device);
        return status;
    }

    //
    // Note: WdfDeviceCreateDeviceInterface is NOT supported for control
    // devices. User-mode accesses us via the symbolic link \\.\KmProcMon.
    //

    status = KernelMonitorDriverQueueInitialize(device);
    if (!NT_SUCCESS(status)) {
        KmDbgError("Queue init failed: 0x%08X", status);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "Queue init failed: 0x%08X", status);
        WdfObjectDelete(device);
        return status;
    }

    //
    // Control device must call this to complete initialization.
    // Only publish g_DeviceContext AFTER full success so notification
    // callbacks never see a half-initialized context.
    //
    WdfControlFinishInitializing(device);
    g_DeviceContext = ctx;
    KmDbg("Control device initialization complete");

    return STATUS_SUCCESS;
}

VOID
DeviceContextPushEvent(
    _In_ PDEVICE_CONTEXT    Ctx,
    _In_ PKMPROCMON_EVENT   Event
    )
{
    KIRQL oldIrql;
    LONG  slot;

    KeAcquireSpinLock(&Ctx->BufferLock, &oldIrql);

    if (Ctx->Count >= KMPROCMON_EVENT_BUFFER_SIZE) {
        InterlockedIncrement64(&Ctx->EventsDropped);
        KmDbgWarn("Ring buffer full, event dropped (total=%lld)", Ctx->EventsDropped);
        KeReleaseSpinLock(&Ctx->BufferLock, oldIrql);
        return;
    }

    slot = Ctx->Head;
    Ctx->Head = (Ctx->Head + 1) % KMPROCMON_EVENT_BUFFER_SIZE;
    InterlockedIncrement(&Ctx->Count);

    RtlCopyMemory(&Ctx->EventBuffer[slot], Event, sizeof(KMPROCMON_EVENT));

    KeReleaseSpinLock(&Ctx->BufferLock, oldIrql);
}
