/*++

Module Name:

    driver.h

Abstract:

    Master include for the kernel process monitor driver.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <wdmsec.h>
#include <initguid.h>
#include <ntstrsafe.h>

#include "device.h"
#include "queue.h"
#include "trace.h"
#include "debug.h"
#include "ProcessMonitor.h"
#include "ObProtect.h"

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD KernelMonitorDriverEvtDriverUnload;
EVT_WDF_OBJECT_CONTEXT_CLEANUP KernelMonitorDriverEvtDriverContextCleanup;

EXTERN_C_END
