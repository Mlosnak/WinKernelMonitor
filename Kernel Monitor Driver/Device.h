/*++

Module Name:

    device.h

Abstract:

    Device context definition for the kernel process monitor.
    Maintains a lock-protected circular buffer that stores telemetry
    events produced by the notification callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

#include "public.h"

EXTERN_C_START

typedef struct _DEVICE_CONTEXT
{
    //
    // Circular event buffer
    //
    KMPROCMON_EVENT     EventBuffer[KMPROCMON_EVENT_BUFFER_SIZE];
    volatile LONG       Head;
    volatile LONG       Tail;
    volatile LONG       Count;
    KSPIN_LOCK          BufferLock;

    //
    // Counters for statistics reporting
    //
    volatile LONGLONG   TotalProcessCreated;
    volatile LONGLONG   TotalProcessExited;
    volatile LONGLONG   TotalThreadCreated;
    volatile LONGLONG   TotalThreadExited;
    volatile LONGLONG   EventsDropped;
    LARGE_INTEGER       StartTime;

    //
    // Runtime config pushed from user-mode
    //
    KMPROCMON_CONFIG    Config;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// We keep a global pointer to the single device context so that
// the notification callbacks (which receive no WDF handle) can
// reach the ring buffer without an expensive lookup.
//
extern PDEVICE_CONTEXT g_DeviceContext;

NTSTATUS
KernelMonitorDriverCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

VOID
DeviceContextPushEvent(
    _In_ PDEVICE_CONTEXT    Ctx,
    _In_ PKMPROCMON_EVENT   Event
    );

EXTERN_C_END
