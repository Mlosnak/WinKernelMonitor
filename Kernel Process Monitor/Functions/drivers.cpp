#include "drivers.h"
#include <Psapi.h>
#include <Softpub.h>
#include <mscat.h>
#include <wintrust.h>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Wintrust.lib")
#pragma comment(lib, "Crypt32.lib")

bool Drivers::VerifySignature(const std::wstring& filePath, std::wstring& signerName)
{
    signerName.clear();
    if (filePath.empty()) return false;

    WINTRUST_FILE_INFO fileInfo{};
    fileInfo.cbStruct       = sizeof(fileInfo);
    fileInfo.pcwszFilePath  = filePath.c_str();
    fileInfo.hFile          = nullptr;
    fileInfo.pgKnownSubject = nullptr;

    GUID actionId = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    WINTRUST_DATA wtd{};
    wtd.cbStruct            = sizeof(wtd);
    wtd.pPolicyCallbackData = nullptr;
    wtd.pSIPClientData      = nullptr;
    wtd.dwUIChoice          = WTD_UI_NONE;
    wtd.fdwRevocationChecks = WTD_REVOKE_NONE;
    wtd.dwUnionChoice       = WTD_CHOICE_FILE;
    wtd.pFile               = &fileInfo;
    wtd.dwStateAction       = WTD_STATEACTION_VERIFY;
    wtd.hWVTStateData       = nullptr;
    wtd.dwProvFlags         = WTD_CACHE_ONLY_URL_RETRIEVAL;

    LONG trustResult = WinVerifyTrust(
        (HWND)INVALID_HANDLE_VALUE, &actionId, &wtd);

    bool isSigned = (trustResult == ERROR_SUCCESS);

    if (isSigned && wtd.hWVTStateData) {
        CRYPT_PROVIDER_DATA* provData =
            WTHelperProvDataFromStateData(wtd.hWVTStateData);
        if (provData) {
            CRYPT_PROVIDER_SGNR* sgnr = WTHelperGetProvSignerFromChain(provData, 0, FALSE, 0);
            if (sgnr && sgnr->csCertChain > 0) {
                CRYPT_PROVIDER_CERT* cert = WTHelperGetProvCertFromChain(sgnr, 0);
                if (cert && cert->pCert) {
                    wchar_t name[256] = {};
                    CertGetNameStringW(cert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
                        0, nullptr, name, 256);
                    signerName = name;
                }
            }
        }
    }

    wtd.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &actionId, &wtd);

    // fallback: catalog signature
    if (!isSigned) {
        HCATADMIN hCatAdmin = nullptr;
        GUID driverActionGuid = DRIVER_ACTION_VERIFY;
        if (CryptCATAdminAcquireContext(&hCatAdmin, &driverActionGuid, 0)) {
            HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ,
                FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
            if (hFile != INVALID_HANDLE_VALUE) {
                DWORD hashSize = 0;
                CryptCATAdminCalcHashFromFileHandle(hFile, &hashSize, nullptr, 0);
                if (hashSize > 0) {
                    std::vector<BYTE> hash(hashSize);
                    if (CryptCATAdminCalcHashFromFileHandle(hFile, &hashSize, hash.data(), 0)) {
                        HCATINFO hCatInfo = CryptCATAdminEnumCatalogFromHash(
                            hCatAdmin, hash.data(), hashSize, 0, nullptr);
                        if (hCatInfo) {
                            isSigned = true;
                            signerName = L"Microsoft (Catalog)";
                            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
                        }
                    }
                }
                CloseHandle(hFile);
            }
            CryptCATAdminReleaseContext(hCatAdmin, 0);
        }
    }

    return isSigned;
}

bool Drivers::Enumerate(std::vector<DriverInfo>& out)
{
    out.clear();

    LPVOID drivers[1024];
    DWORD cbNeeded = 0;
    if (!EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded))
        return false;

    DWORD count = cbNeeded / sizeof(LPVOID);

    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    std::wstring sysRoot(sysDir);
    // Strip \system32 to get Windows root
    auto pos = sysRoot.rfind(L'\\');
    std::wstring winRoot = (pos != std::wstring::npos) ? sysRoot.substr(0, pos) : sysRoot;

    for (DWORD i = 0; i < count && i < 1024; i++) {
        if (!drivers[i]) continue;

        wchar_t baseName[256] = {};
        wchar_t fileName[512] = {};

        GetDeviceDriverBaseNameW(drivers[i], baseName, 256);
        GetDeviceDriverFileNameW(drivers[i], fileName, 512);

        DriverInfo di{};
        di.Name         = baseName;
        di.BaseAddress  = drivers[i];
        di.ImageSize    = 0;

        std::wstring path = fileName;
        if (path.starts_with(L"\\SystemRoot\\"))
            path = winRoot + path.substr(11);
        else if (path.starts_with(L"\\??\\"))
            path = path.substr(4);
        di.Path = path;

        di.Signed = VerifySignature(di.Path, di.Signer);

        out.push_back(std::move(di));
    }

    return true;
}
