#pragma once

#include <Windows.h>
#include <string>
#include <vector>

struct DriverInfo {
    std::wstring    Name;
    std::wstring    Path;
    void*           BaseAddress;
    DWORD           ImageSize;
    bool            Signed;
    std::wstring    Signer;
};

namespace Drivers {
    bool Enumerate(std::vector<DriverInfo>& out);
    bool VerifySignature(const std::wstring& filePath, std::wstring& signerName);
}
