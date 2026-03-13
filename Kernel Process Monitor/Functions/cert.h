#pragma once

#include <Windows.h>
#include <string>

enum class CertResult {
    Trusted,
    UntrustedRoot,
    Expired,
    Unsigned,
    BadSignature,
    Failed
};

namespace Cert {
    CertResult      Check(const std::wstring& path);
    const wchar_t*  Str(CertResult r);
}
