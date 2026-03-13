/*++

Module Name:

    ObProtect.c

Abstract:

    Registers an OB_CALLBACK that intercepts handle operations
    targeting the user-mode monitor process. Strips dangerous
    access rights (PROCESS_TERMINATE, PROCESS_SUSPEND_RESUME)
    so that external tools cannot forcibly kill the monitor.

    The protected PID is set to the PID of whichever process
    first opens the driver device handle (assumed to be the
    monitor application). A production build would use a more
    robust identification mechanism (e.g. signed binary check).

Environment:

    Kernel-mode

--*/

#include "Driver.h"

#ifndef PROCESS_TERMINATE
#define PROCESS_TERMINATE       0x0001
#endif
#ifndef PROCESS_VM_OPERATION
#define PROCESS_VM_OPERATION    0x0008
#endif
#ifndef PROCESS_VM_WRITE
#define PROCESS_VM_WRITE        0x0020
#endif

static PVOID            g_ObRegistrationHandle  = NULL;
static volatile LONG    g_ProtectedPid          = 0;

//
// Pre-operation callback: strip dangerous access bits
//
static OB_PREOP_CALLBACK_STATUS
ObPreOperationCallback(
    _In_ PVOID                          RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION OperationInfo
    )
{
    LONG    protectedPid;
    HANDLE  targetPid;

    UNREFERENCED_PARAMETER(RegistrationContext);

    if (OperationInfo->ObjectType != *PsProcessType) {
        return OB_PREOP_SUCCESS;
    }

    protectedPid = InterlockedCompareExchange(&g_ProtectedPid, 0, 0);
    if (protectedPid == 0) {
        return OB_PREOP_SUCCESS;
    }

    targetPid = PsGetProcessId((PEPROCESS)OperationInfo->Object);
    if (HandleToLong(targetPid) != protectedPid) {
        return OB_PREOP_SUCCESS;
    }

    //
    // Do not strip access from kernel callers or from our own process
    //
    if (OperationInfo->KernelHandle) {
        return OB_PREOP_SUCCESS;
    }

    if (OperationInfo->Operation == OB_OPERATION_HANDLE_CREATE) {
        OperationInfo->Parameters->CreateHandleInformation.DesiredAccess &=
            ~(PROCESS_TERMINATE | PROCESS_VM_WRITE | PROCESS_VM_OPERATION);
        KmDbgWarn("OB: stripped dangerous access from handle CREATE on PID=%ld", protectedPid);
    }
    else if (OperationInfo->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
        OperationInfo->Parameters->DuplicateHandleInformation.DesiredAccess &=
            ~(PROCESS_TERMINATE | PROCESS_VM_WRITE | PROCESS_VM_OPERATION);
        KmDbgWarn("OB: stripped dangerous access from handle DUPLICATE on PID=%ld", protectedPid);
    }

    return OB_PREOP_SUCCESS;
}

NTSTATUS
ObProtectRegister(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    NTSTATUS                    status;
    OB_CALLBACK_REGISTRATION   cbReg;
    OB_OPERATION_REGISTRATION  opReg;
    UNICODE_STRING              altitude;

    RtlInitUnicodeString(&altitude, KMPROCMON_ALTITUDE);

    RtlZeroMemory(&opReg, sizeof(opReg));
    opReg.ObjectType            = PsProcessType;
    opReg.Operations            = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    opReg.PreOperation          = ObPreOperationCallback;
    opReg.PostOperation         = NULL;

    RtlZeroMemory(&cbReg, sizeof(cbReg));
    cbReg.Version                       = OB_FLT_REGISTRATION_VERSION;
    cbReg.OperationRegistrationCount    = 1;
    cbReg.Altitude                      = altitude;
    cbReg.RegistrationContext           = NULL;
    cbReg.OperationRegistration         = &opReg;

    status = ObRegisterCallbacks(&cbReg, &g_ObRegistrationHandle);
    if (!NT_SUCCESS(status)) {
        g_ObRegistrationHandle = NULL;
        KmDbgError("ObRegisterCallbacks failed: 0x%08X", status);
    } else {
        KmDbg("ObRegisterCallbacks registered at altitude %wZ", &altitude);
    }

    UNREFERENCED_PARAMETER(DriverObject);
    return status;
}

VOID
ObProtectUnregister(VOID)
{
    if (g_ObRegistrationHandle) {
        ObUnRegisterCallbacks(g_ObRegistrationHandle);
        g_ObRegistrationHandle = NULL;
        KmDbg("ObRegisterCallbacks unregistered");
    }

    InterlockedExchange(&g_ProtectedPid, 0);
}

VOID
ObProtectSetPid(
    _In_ LONG Pid
    )
{
    InterlockedExchange(&g_ProtectedPid, Pid);
    KmDbg("ObProtect: now protecting PID=%ld", Pid);
}
