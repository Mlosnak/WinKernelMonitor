#include "pe.h"
#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")

bool Pe::Walk(DWORD pid, std::vector<PeModule>& out)
{
    out.clear();

    HANDLE hp = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hp) return false;

    HMODULE mods[1024]{};
    DWORD cb = 0;

    if (!EnumProcessModulesEx(hp, mods, sizeof(mods), &cb, LIST_MODULES_ALL)) {
        CloseHandle(hp);
        return false;
    }

    DWORD count = cb / sizeof(HMODULE);
    out.reserve(count);

    for (DWORD i = 0; i < count; i++) {
        PeModule m{};
        m.Base = mods[i];

        WCHAR name[MAX_PATH]{};
        WCHAR path[MAX_PATH]{};
        GetModuleBaseNameW(hp, mods[i], name, MAX_PATH);
        GetModuleFileNameExW(hp, mods[i], path, MAX_PATH);

        MODULEINFO mi{};
        if (GetModuleInformation(hp, mods[i], &mi, sizeof(mi)))
            m.Size = mi.SizeOfImage;

        m.Name = name;
        m.Path = path;
        out.push_back(std::move(m));
    }

    CloseHandle(hp);
    return true;
}
