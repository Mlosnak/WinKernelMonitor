/*++

Module Name:

    public.h

Abstract:

    Public interface GUID and shared includes for the kernel monitor driver.

Environment:

    Kernel-mode and User-mode

--*/

#pragma once

#include "../Shared/KmProcMonShared.h"

DEFINE_GUID(GUID_DEVINTERFACE_KernelMonitorDriver,
    0xdf031514, 0x5938, 0x4cd9, 0xab, 0x2b, 0x94, 0x16, 0x16, 0x9d, 0x8d, 0x91);
// {df031514-5938-4cd9-ab2b-9416169d8d91}
