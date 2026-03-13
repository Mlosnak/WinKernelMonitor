#include "kmio.h"
#include "../Logs/dbglog.h"
#include <format>

static constexpr DWORD POLL_BUFFER_SIZE =
    sizeof(KMPROCMON_EVENT_BATCH) + (64 * sizeof(KMPROCMON_EVENT));

KmIo::KmIo() {}

KmIo::~KmIo() { Close(); }

bool KmIo::Open()
{
    if (m_hDevice != INVALID_HANDLE_VALUE) return true;

    m_hDevice = CreateFileW(
        KMPROCMON_WIN32_DEVICE,
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (m_hDevice == INVALID_HANDLE_VALUE) {
        m_dwErr = GetLastError();
        DbgLog::Write(LogSev::Err, std::format(L"CreateFile({}) failed: {}",
            KMPROCMON_WIN32_DEVICE, m_dwErr));
        return false;
    }

    DbgLog::Write(LogSev::Ok, L"Device handle acquired");
    return true;
}

void KmIo::Close()
{
    if (m_hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hDevice);
        m_hDevice = INVALID_HANDLE_VALUE;
    }
}

bool KmIo::Ioctl(DWORD code, void* in, DWORD inSz, void* out, DWORD outSz, DWORD* ret)
{
    if (m_hDevice == INVALID_HANDLE_VALUE) return false;
    DWORD bytes = 0;
    BOOL ok = DeviceIoControl(m_hDevice, code, in, inSz, out, outSz, &bytes, nullptr);
    if (ret) *ret = bytes;
    if (!ok) { m_dwErr = GetLastError(); return false; }
    return true;
}

bool KmIo::PollEvents(std::vector<KMPROCMON_EVENT>& out, ULONG& dropped)
{
    std::vector<BYTE> raw(POLL_BUFFER_SIZE, 0);
    DWORD bytes = 0;

    if (!Ioctl(IOCTL_KMPROCMON_GET_EVENTS, nullptr, 0,
               raw.data(), static_cast<DWORD>(raw.size()), &bytes))
        return false;

    auto* batch = reinterpret_cast<PKMPROCMON_EVENT_BATCH>(raw.data());
    dropped = batch->Dropped;
    out.clear();
    out.reserve(batch->Count);
    for (ULONG i = 0; i < batch->Count; i++)
        out.push_back(batch->Events[i]);
    return true;
}

bool KmIo::Flush()
{
    return Ioctl(IOCTL_KMPROCMON_CLEAR_EVENTS, nullptr, 0, nullptr, 0, nullptr);
}

bool KmIo::QueryStats(KMPROCMON_STATS& stats)
{
    DWORD bytes = 0;
    return Ioctl(IOCTL_KMPROCMON_GET_STATS, nullptr, 0, &stats, sizeof(stats), &bytes);
}

bool KmIo::PushConfig(const KMPROCMON_CONFIG& cfg)
{
    KMPROCMON_CONFIG copy = cfg;
    return Ioctl(IOCTL_KMPROCMON_SET_CONFIG, &copy, sizeof(copy), nullptr, 0, nullptr);
}

bool KmIo::SetProtectedPid(DWORD pid)
{
    ULONG p = pid;
    return Ioctl(IOCTL_KMPROCMON_SET_PROTECTED_PID, &p, sizeof(p), nullptr, 0, nullptr);
}
