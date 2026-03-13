/*++

Module Name:

    ObProtect.h

Abstract:

    Anti-tampering layer using ObRegisterCallbacks.
    Strips PROCESS_TERMINATE from handles opened to the
    user-mode monitor process, preventing forced kills.

Environment:

    Kernel-mode

--*/

#pragma once

EXTERN_C_START

NTSTATUS
ObProtectRegister(
    _In_ PDRIVER_OBJECT DriverObject
    );

VOID
ObProtectUnregister(VOID);

VOID
ObProtectSetPid(
    _In_ LONG Pid
    );

EXTERN_C_END
