#include "dbglog.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <format>

static std::mutex       s_Lock;
static std::wofstream   s_File;
static LogSev           s_MinLevel = LogSev::Ok;
static bool             s_Ready    = false;

static const wchar_t* SevTag(LogSev s)
{
    switch (s) {
    case LogSev::Trace: return L"TRC";
    case LogSev::Ok:    return L"INF";
    case LogSev::Warn:  return L"WRN";
    case LogSev::Err:   return L"ERR";
    case LogSev::Fatal: return L"FTL";
    default:            return L"???";
    }
}

static std::wstring Now()
{
    auto tp = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    struct tm lt{};
    localtime_s(&lt, &tt);
    std::wostringstream o;
    o << std::put_time(&lt, L"%H:%M:%S") << L"." << std::setfill(L'0') << std::setw(3) << ms.count();
    return o.str();
}

void DbgLog::Init(const std::wstring& filePath, LogSev minLevel)
{
    std::lock_guard<std::mutex> g(s_Lock);
    if (s_Ready) return;
    s_MinLevel = minLevel;
    if (!filePath.empty()) {
        s_File.open(filePath, std::ios::out | std::ios::app);
    }
    s_Ready = true;
}

void DbgLog::Shutdown()
{
    std::lock_guard<std::mutex> g(s_Lock);
    if (s_File.is_open()) { s_File.flush(); s_File.close(); }
    s_Ready = false;
}

void DbgLog::SetLevel(LogSev sev) { s_MinLevel = sev; }

void DbgLog::Write(LogSev sev, const std::wstring& msg)
{
    if (static_cast<int>(sev) < static_cast<int>(s_MinLevel)) return;
    auto line = std::format(L"[{}] [{}] {}", Now(), SevTag(sev), msg);

    std::lock_guard<std::mutex> g(s_Lock);
    std::wcout << line << std::endl;
    if (s_File.is_open()) s_File << line << std::endl;

    OutputDebugStringW(line.c_str());
    OutputDebugStringW(L"\n");
}

void DbgLog::Write(LogSev sev, const std::wstring& tag, const std::wstring& msg)
{
    if (static_cast<int>(sev) < static_cast<int>(s_MinLevel)) return;
    auto line = std::format(L"[{}] [{}] [{}] {}", Now(), SevTag(sev), tag, msg);

    std::lock_guard<std::mutex> g(s_Lock);
    std::wcout << line << std::endl;
    if (s_File.is_open()) s_File << line << std::endl;

    OutputDebugStringW(line.c_str());
    OutputDebugStringW(L"\n");
}
