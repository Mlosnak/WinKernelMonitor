#pragma once

#include <Windows.h>
#include <string>
#include <mutex>
#include <fstream>

enum class LogSev : int {
    Trace = 0,
    Ok    = 1,
    Warn  = 2,
    Err   = 3,
    Fatal = 4
};

namespace DbgLog {

    void    Init(const std::wstring& filePath, LogSev minLevel = LogSev::Ok);
    void    Shutdown();
    void    Write(LogSev sev, const std::wstring& msg);
    void    Write(LogSev sev, const std::wstring& tag, const std::wstring& msg);
    void    SetLevel(LogSev sev);

}
