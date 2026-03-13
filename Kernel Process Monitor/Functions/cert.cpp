#include "cert.h"
#include <Softpub.h>
#include <wintrust.h>

#pragma comment(lib, "wintrust.lib")

CertResult Cert::Check(const std::wstring& path)
{
    if (path.empty()) return CertResult::Failed;

    GUID actionId = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    WINTRUST_FILE_INFO fi{};
    fi.cbStruct      = sizeof(fi);
    fi.pcwszFilePath = path.c_str();

    WINTRUST_DATA wd{};
    wd.cbStruct            = sizeof(wd);
    wd.dwUIChoice          = WTD_UI_NONE;
    wd.fdwRevocationChecks = WTD_REVOKE_NONE;
    wd.dwUnionChoice       = WTD_CHOICE_FILE;
    wd.dwStateAction       = WTD_STATEACTION_VERIFY;
    wd.dwProvFlags         = WTD_SAFER_FLAG;
    wd.pFile               = &fi;

    LONG r = WinVerifyTrust(
        static_cast<HWND>(INVALID_HANDLE_VALUE), &actionId, &wd);

    wd.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &actionId, &wd);

    switch (r) {
    case ERROR_SUCCESS:                                     return CertResult::Trusted;
    case TRUST_E_NOSIGNATURE:                               return CertResult::Unsigned;
    case TRUST_E_EXPLICIT_DISTRUST:
    case TRUST_E_SUBJECT_NOT_TRUSTED:                       return CertResult::BadSignature;
    case CERT_E_UNTRUSTEDROOT:
    case CERT_E_CHAINING:                                   return CertResult::UntrustedRoot;
    case CERT_E_EXPIRED:                                    return CertResult::Expired;
    default:                                                return CertResult::Failed;
    }
}

const wchar_t* Cert::Str(CertResult r)
{
    switch (r) {
    case CertResult::Trusted:       return L"Signed";
    case CertResult::UntrustedRoot: return L"Untrusted Root";
    case CertResult::Expired:       return L"Expired";
    case CertResult::Unsigned:      return L"Unsigned";
    case CertResult::BadSignature:  return L"Bad Signature";
    case CertResult::Failed:        return L"Check Failed";
    default:                        return L"Unknown";
    }
}
