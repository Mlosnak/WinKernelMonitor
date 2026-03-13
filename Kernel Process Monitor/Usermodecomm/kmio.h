#pragma once

#include <Windows.h>
#include <vector>
#include "../../Shared/KmProcMonShared.h"

class KmIo {
public:
    KmIo();
    ~KmIo();

    KmIo(const KmIo&) = delete;
    KmIo& operator=(const KmIo&) = delete;

    bool    Open();
    void    Close();
    bool    IsOpen() const { return m_hDevice != INVALID_HANDLE_VALUE; }

    bool    PollEvents(std::vector<KMPROCMON_EVENT>& out, ULONG& dropped);
    bool    Flush();
    bool    QueryStats(KMPROCMON_STATS& stats);
    bool    PushConfig(const KMPROCMON_CONFIG& cfg);
    bool    SetProtectedPid(DWORD pid);

    DWORD   LastErr() const { return m_dwErr; }

private:
    bool    Ioctl(DWORD code, void* in, DWORD inSz, void* out, DWORD outSz, DWORD* ret);

    HANDLE  m_hDevice = INVALID_HANDLE_VALUE;
    DWORD   m_dwErr   = 0;
};
