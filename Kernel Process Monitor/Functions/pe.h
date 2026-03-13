#pragma once

#include <Windows.h>
#include <string>
#include <vector>

struct PeModule {
    std::wstring    Name;
    std::wstring    Path;
    HMODULE         Base;
    DWORD           Size;
};

namespace Pe {
    bool Walk(DWORD pid, std::vector<PeModule>& out);
}
