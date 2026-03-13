/*++

Module Name:

    driver.c

Abstract:

    Driver entry point. Registers PsSetCreateProcessNotifyRoutineEx,
    PsSetCreateThreadNotifyRoutine, and ObRegisterCallbacks during
    device add, and tears them down on cleanup.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, KernelMonitorDriverEvtDriverContextCleanup)
#endif

static BOOLEAN g_ProcessNotifyRegistered    = FALSE;
static BOOLEAN g_ThreadNotifyRegistered     = FALSE;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG       config;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDRIVER               driver;
    PWDFDEVICE_INIT         deviceInit;
    NTSTATUS                status;

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    KmDbg("DriverEntry: initializing driver");
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = KernelMonitorDriverEvtDriverContextCleanup;

    //
    // Non-PnP driver: set EvtDriverDeviceAdd to NULL
    //
    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    config.EvtDriverUnload = KernelMonitorDriverEvtDriverUnload;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes,
        &config,
        &driver
        );

    if (!NT_SUCCESS(status)) {
        KmDbgError("WdfDriverCreate failed: 0x%08X", status);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
            "WdfDriverCreate failed: 0x%08X", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    KmDbg("DriverEntry: WdfDriverCreate succeeded");

    //
    // Allocate a WDFDEVICE_INIT for our control device
    // SDDL: System=all, Administrators=all
    //
    DECLARE_CONST_UNICODE_STRING(sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
    deviceInit = WdfControlDeviceInitAllocate(driver, &sddl);
    if (deviceInit == NULL) {
        KmDbgError("WdfControlDeviceInitAllocate failed");
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
            "WdfControlDeviceInitAllocate failed");
        WPP_CLEANUP(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create the control device and register callbacks
    //
    status = KernelMonitorDriverCreateDevice(deviceInit);
    if (!NT_SUCCESS(status)) {
        KmDbgError("CreateDevice failed: 0x%08X", status);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
            "CreateDevice failed: 0x%08X", status);
        //
        // Do NOT call WdfDeviceInitFree here.
        // WdfDeviceCreate() inside CreateDevice consumes deviceInit
        // regardless of success or failure.
        //
        WPP_CLEANUP(DriverObject);
        return status;
    }

    //
    // Register process creation/exit callback
    //
    status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallback, FALSE);
    if (!NT_SUCCESS(status)) {
        KmDbgError("PsSetCreateProcessNotifyRoutineEx failed: 0x%08X", status);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
            "PsSetCreateProcessNotifyRoutineEx failed: 0x%08X", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }
    g_ProcessNotifyRegistered = TRUE;
    KmDbg("Process notification callback registered");

    //
    // Register thread creation/exit callback
    //
    status = PsSetCreateThreadNotifyRoutine(ThreadNotifyCallback);
    if (!NT_SUCCESS(status)) {
        KmDbgWarn("PsSetCreateThreadNotifyRoutine failed: 0x%08X (non-fatal)", status);
        TraceEvents(TRACE_LEVEL_WARNING, TRACE_DRIVER,
            "PsSetCreateThreadNotifyRoutine failed: 0x%08X (non-fatal)", status);
    } else {
        g_ThreadNotifyRegistered = TRUE;
        KmDbg("Thread notification callback registered");
    }

    //
    // Anti-tampering via ObRegisterCallbacks
    //
    status = ObProtectRegister(DriverObject);
    if (!NT_SUCCESS(status)) {
        KmDbgWarn("ObProtectRegister failed: 0x%08X (non-fatal)", status);
        TraceEvents(TRACE_LEVEL_WARNING, TRACE_DRIVER,
            "ObProtectRegister failed: 0x%08X (non-fatal)", status);
    } else {
        KmDbg("ObRegisterCallbacks anti-tamper active");
    }

    KmDbg("DriverEntry: driver initialized successfully");
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");
    return STATUS_SUCCESS;
}

VOID
KernelMonitorDriverEvtDriverUnload(
    _In_ WDFDRIVER Driver
    )
{
    UNREFERENCED_PARAMETER(Driver);
    KmDbg("DriverUnload called");
}

VOID
KernelMonitorDriverEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
{
    PAGED_CODE();

    KmDbg("Driver cleanup: tearing down callbacks");
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    ObProtectUnregister();

    if (g_ThreadNotifyRegistered) {
        PsRemoveCreateThreadNotifyRoutine(ThreadNotifyCallback);
        g_ThreadNotifyRegistered = FALSE;
    }

    if (g_ProcessNotifyRegistered) {
        PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallback, TRUE);
        g_ProcessNotifyRegistered = FALSE;
    }

    g_DeviceContext = NULL;

    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}
