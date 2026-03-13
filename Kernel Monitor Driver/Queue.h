/*++

Module Name:

    queue.h

Abstract:

    I/O queue definitions for IOCTL dispatch.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

EXTERN_C_START

NTSTATUS
KernelMonitorDriverQueueInitialize(
    _In_ WDFDEVICE Device
    );

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL KernelMonitorDriverEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP KernelMonitorDriverEvtIoStop;

EXTERN_C_END
