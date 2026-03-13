/*++

Module Name:

    debug.h

Abstract:

    DbgPrintEx macros for DbgView output. Works in both
    Debug and Release builds when the kernel debugger or
    DbgView is attached with verbose kernel output enabled.

Environment:

    Kernel-mode

--*/

#pragma once

#include <ntddk.h>

#define KMPROCMON_PREFIX        "[KmProcMon] "

#define KmDbg(fmt, ...)         \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,    \
               KMPROCMON_PREFIX fmt "\n", __VA_ARGS__)

#define KmDbgError(fmt, ...)    \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,   \
               KMPROCMON_PREFIX "[!] " fmt "\n", __VA_ARGS__)

#define KmDbgWarn(fmt, ...)     \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, \
               KMPROCMON_PREFIX "[*] " fmt "\n", __VA_ARGS__)

#define KmDbgTrace(fmt, ...)    \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,   \
               KMPROCMON_PREFIX fmt "\n", __VA_ARGS__)
