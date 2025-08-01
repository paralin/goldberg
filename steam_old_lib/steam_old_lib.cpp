#include "steam_old_lib.hpp"

#include "common_helpers/common_helpers.hpp"

#include "detours/detours.h"

#include <string>
#include <stdexcept>
#include <string_view>
#include <cstdint>

#include <Softpub.h> // WinVerifyTrust


#define S_API extern "C" __declspec(dllexport)
#define STEAM_CALL __cdecl

#define STEAM_INVALID_CALL_HANDLE 0


static HMODULE my_hModule = nullptr;
HMODULE wintrust_dll = nullptr;
std::wstring wintrust_lib_path = L"";

static bool WinVerifyTrust_hooked = false;
static bool LoadLibraryA_hooked = false;
static bool LoadLibraryExA_hooked = false;
static bool LoadLibraryW_hooked = false;
static bool LoadLibraryExW_hooked = false;

static decltype(WinVerifyTrust) *actual_WinVerifyTrust = nullptr;
__declspec(noinline)
static LONG WINAPI WinVerifyTrust_hook(HWND hwnd, GUID *pgActionID, LPVOID pWVTData) {
    if (WinVerifyTrust_hooked) {
        SetLastError(ERROR_SUCCESS);
        return 0; // success
    }

    if (actual_WinVerifyTrust) {
        return actual_WinVerifyTrust(hwnd, pgActionID, pWVTData);
    }
    
    SetLastError(ERROR_SUCCESS);
    return 0; // success
}

static decltype(LoadLibraryA) *actual_LoadLibraryA = LoadLibraryA;
__declspec(noinline)
static HMODULE WINAPI LoadLibraryA_hook(LPCSTR lpLibFileName)
{
    if (LoadLibraryA_hooked &&
        lpLibFileName && lpLibFileName[0] &&
        common_helpers::ends_with_i(lpLibFileName, "Steam.dll")) {
        return my_hModule;
    }

    return actual_LoadLibraryA(lpLibFileName);
}

static decltype(LoadLibraryExA) *actual_LoadLibraryExA = LoadLibraryExA;
__declspec(noinline)
static HMODULE WINAPI LoadLibraryExA_hook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    if (LoadLibraryExA_hooked &&
        lpLibFileName && lpLibFileName[0] &&
        common_helpers::ends_with_i(lpLibFileName, "Steam.dll")) {
        return my_hModule;
    }

    return actual_LoadLibraryExA(lpLibFileName, hFile, dwFlags);
}

static decltype(LoadLibraryW) *actual_LoadLibraryW = LoadLibraryW;
__declspec(noinline)
static HMODULE WINAPI LoadLibraryW_hook(LPCWSTR lpLibFileName)
{
    if (LoadLibraryW_hooked &&
        lpLibFileName && lpLibFileName[0] &&
        common_helpers::ends_with_i(lpLibFileName, L"Steam.dll")) {
        return my_hModule;
    }

    return actual_LoadLibraryW(lpLibFileName);
}

static decltype(LoadLibraryExW) *actual_LoadLibraryExW = LoadLibraryExW;
__declspec(noinline)
static HMODULE WINAPI LoadLibraryExW_hook(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    if (LoadLibraryExW_hooked &&
        lpLibFileName && lpLibFileName[0] &&
        common_helpers::ends_with_i(lpLibFileName, L"Steam.dll")) {
        return my_hModule;
    }

    return actual_LoadLibraryExW(lpLibFileName, hFile, dwFlags);
}


static bool redirect_win32_apis()
{
    try {
        wintrust_dll = LoadLibraryExW(wintrust_lib_path.c_str(), NULL, 0);
        if (!wintrust_dll) throw std::runtime_error("");

        actual_WinVerifyTrust = (decltype(actual_WinVerifyTrust))GetProcAddress(wintrust_dll, "WinVerifyTrust");
        if (!actual_WinVerifyTrust) throw std::runtime_error("");

        if (DetourTransactionBegin() != NO_ERROR) throw std::runtime_error("");
        if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR) throw std::runtime_error("");

        if (DetourAttach((PVOID *)&actual_WinVerifyTrust, (PVOID)WinVerifyTrust_hook) != NO_ERROR) throw std::runtime_error("");
        if (DetourAttach((PVOID *)&actual_LoadLibraryA, (PVOID)LoadLibraryA_hook) != NO_ERROR) throw std::runtime_error("");
        if (DetourAttach((PVOID *)&actual_LoadLibraryExA, (PVOID)LoadLibraryExA_hook) != NO_ERROR) throw std::runtime_error("");
        if (DetourAttach((PVOID *)&actual_LoadLibraryW, (PVOID)LoadLibraryW_hook) != NO_ERROR) throw std::runtime_error("");
        if (DetourAttach((PVOID *)&actual_LoadLibraryExW, (PVOID)LoadLibraryExW_hook) != NO_ERROR) throw std::runtime_error("");
        if (DetourTransactionCommit() != NO_ERROR) throw std::runtime_error("");

        WinVerifyTrust_hooked = true;
        LoadLibraryA_hooked = true;
        LoadLibraryExA_hooked = true;
        LoadLibraryW_hooked = true;
        LoadLibraryExW_hooked = true;

        return true;
    } catch (...) {

    }

    if (wintrust_dll) {
        FreeLibrary(wintrust_dll);
        wintrust_dll = nullptr;
    }

    return false;
}

static bool restore_win32_apis()
{
    WinVerifyTrust_hooked = false;
    LoadLibraryA_hooked = false;
    LoadLibraryExA_hooked = false;
    LoadLibraryW_hooked = false;
    LoadLibraryExW_hooked = false;

    bool ret = false;

    try {
        if (DetourTransactionBegin() != NO_ERROR) throw std::runtime_error("");
        if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR) throw std::runtime_error("");

        if (actual_WinVerifyTrust) {
            DetourDetach((PVOID *)&actual_WinVerifyTrust, (PVOID)WinVerifyTrust_hook);
        }

        DetourDetach((PVOID *)&actual_LoadLibraryA, (PVOID)LoadLibraryA_hook);
        DetourDetach((PVOID *)&actual_LoadLibraryExA, (PVOID)LoadLibraryExA_hook);
        DetourDetach((PVOID *)&actual_LoadLibraryW, (PVOID)LoadLibraryW_hook);
        DetourDetach((PVOID *)&actual_LoadLibraryExW, (PVOID)LoadLibraryExW_hook);
        if (DetourTransactionCommit() != NO_ERROR) throw std::runtime_error("");

        ret = true;
    } catch (...) {

    }

    if (wintrust_dll) {
        FreeLibrary(wintrust_dll);
        wintrust_dll = nullptr;
    }

    return ret;
}

#ifdef SOLD_EXTRA_DEBUG
extern "C" __declspec(dllexport)
#endif
void __cdecl notify_patch_done()
{
#ifdef SOLD_EXTRA_DEBUG
    MessageBoxA(nullptr, "Cleanup/Uninstall was called, hook a debugger here!", "Cleanup called", MB_OK | MB_ICONASTERISK);
#endif
}

static void set_ok_err(soldlib::TSteamError *pError) {
    if (pError) {
        pError->eSteamError = soldlib::ESteamError::eSteamErrorNone;
        pError->eDetailedErrorType = soldlib::EDetailedPlatformErrorType::eNoDetailedErrorAvailable;
        pError->nDetailedErrorCode = 0;
        pError->szDesc[0] = 0;
    }
}

static void set_bad_err(soldlib::TSteamError *pError) {
    if (pError) {
        pError->eSteamError = soldlib::ESteamError::eSteamErrorBadArg;
        pError->eDetailedErrorType = soldlib::EDetailedPlatformErrorType::eNoDetailedErrorAvailable;
        pError->nDetailedErrorCode = soldlib::ESteamError::eSteamErrorBadArg;
        pError->szDesc[0] = 0;
    }
}

bool soldlib::patch(void *lib_hModule)
{
    my_hModule = (HMODULE)lib_hModule;

    if (wintrust_lib_path.empty()) {
        auto size = GetSystemDirectoryW(&wintrust_lib_path[0], 0);
        if (size <= 0) return false;

        wintrust_lib_path.resize(size);
        size = GetSystemDirectoryW(&wintrust_lib_path[0], (unsigned)wintrust_lib_path.size());
        if (size >= (unsigned)wintrust_lib_path.size()) {
            wintrust_lib_path.clear();
            return false;
        }

        wintrust_lib_path.pop_back(); // remove null
        wintrust_lib_path += L"\\Wintrust.dll";
    }

    return redirect_win32_apis();
}

bool soldlib::restore()
{
    return restore_win32_apis();
}



S_API int STEAM_CALL SteamStartup( unsigned int uUsingMask, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return 1;
}

S_API int STEAM_CALL SteamCleanup( soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    // restore_win32_apis(); // appid 7450 loads the dll again at runtime, we can't unload here
    notify_patch_done();
    return 1;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamUninstall( soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    // restore_win32_apis(); // appid 7450 loads the dll again at runtime, we can't unload here
    notify_patch_done();
    return STEAM_INVALID_CALL_HANDLE;
}

S_API int STEAM_CALL SteamGetAccountStatus( unsigned int* puAccountStatusFlags, soldlib::TSteamError *pError )
{
    if (puAccountStatusFlags) {
        *puAccountStatusFlags =
            soldlib::ESteamAccountStatusBitFields::eSteamAccountStatusDefault
            | soldlib::ESteamAccountStatusBitFields::eSteamAccountStatusEmailVerified;
    }

    set_ok_err(pError);
    return 1;
}

S_API int STEAM_CALL SteamGetCurrentEmailAddress( char *szEmailaddress, unsigned int uBufSize, unsigned int *puEmailaddressChars, soldlib::TSteamError *pError )
{
    constexpr const static std::string_view EMAIL = "orca@gbe.com";

    unsigned copied = 0;
    if (szEmailaddress && uBufSize > 0) {
        copied = (unsigned)EMAIL.copy(szEmailaddress, uBufSize);
        szEmailaddress[copied] = 0;
    }
    if (puEmailaddressChars) *puEmailaddressChars = copied;
    set_ok_err(pError);
    return 1;
}

// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/common/Steam.h
// https://github.com/SteamRE/open-steamworks/blob/master/Open%20Steamworks/Steam.h
S_API int STEAM_CALL SteamIsAppSubscribed( unsigned int uAppId, int *pbIsAppSubscribed, int *pbIsSubscriptionPending, soldlib::TSteamError *pError ) {
    if (pbIsAppSubscribed) *pbIsAppSubscribed = 1;
    if (pbIsSubscriptionPending) *pbIsSubscriptionPending = 0;
    set_ok_err(pError);
    return 1;
}

S_API int STEAM_CALL SteamIsSubscribed( unsigned int uSubscriptionId, int *pbIsSubscribed, int *pbIsSubscriptionPending, soldlib::TSteamError *pError )
{
    if (pbIsSubscribed) *pbIsSubscribed = 1;
    if (pbIsSubscriptionPending) *pbIsSubscriptionPending = 0;
    set_ok_err(pError);
    return 1;
}

S_API int STEAM_CALL SteamIsLoggedIn( int *pbIsLoggedIn, soldlib::TSteamError *pError )
{
    if (pbIsLoggedIn) *pbIsLoggedIn = 1;
    set_ok_err(pError);
    return 1;
}

S_API int STEAM_CALL SteamIsSecureComputer( int *pbIsSecureComputer, soldlib::TSteamError *pError )
{
    if (pbIsSecureComputer) *pbIsSecureComputer = 1;
    set_ok_err(pError);
    return 1;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamLaunchApp( unsigned int uAppId, unsigned int uLaunchOption, const char *cszArgs, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return STEAM_INVALID_CALL_HANDLE;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamLogin( const char *cszUser, const char *cszPassphrase, int bIsSecureComputer, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return STEAM_INVALID_CALL_HANDLE;
}


S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamRefreshAccountInfo( soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return STEAM_INVALID_CALL_HANDLE;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamRefreshAccountInfoEx( int bContentDescriptionOnly, soldlib::TSteamError* pError )
{
    set_ok_err(pError);
    return STEAM_INVALID_CALL_HANDLE;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamRefreshAccountInfo2( int bRefreshAccount, int bRefreshCDDB, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return 1;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamRefreshLogin( const char *cszPassphrase, int bIsSecureComputer, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return STEAM_INVALID_CALL_HANDLE;
}


S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamSubscribe( unsigned int uSubscriptionId, const void /*TSteamSubscriptionBillingInfo*/ *pSubscriptionBillingInfo, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return STEAM_INVALID_CALL_HANDLE;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamUnsubscribe( unsigned int uSubscriptionId, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return 1;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamUpdateAccountBillingInfo( const void /*TSteamPaymentCardInfo*/ *pPaymentCardInfo, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return STEAM_INVALID_CALL_HANDLE;
}

S_API unsigned /*SteamCallHandle_t*/ STEAM_CALL SteamUpdateSubscriptionBillingInfo( unsigned int uSubscriptionId, const void /*TSteamSubscriptionBillingInfo*/ *pSubscriptionBillingInfo, soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return STEAM_INVALID_CALL_HANDLE;
}

S_API unsigned int STEAM_CALL SteamNumAppsRunning( soldlib::TSteamError *pError )
{
    set_ok_err(pError);
    return 1;
}

S_API int STEAM_CALL SteamCheckAppOwnership( unsigned int uAppId, int* pbOwned, void /*TSteamGlobalUserID*/ *pSteamID, soldlib::TSteamError* pError )
{
    if (pbOwned) *pbOwned = 1;
    set_ok_err(pError);
    return 1;
}


S_API int STEAM_CALL SteamGetAppDLCStatus( unsigned int uAppId, unsigned int uDLCCacheId, int *pbDownloaded, soldlib::TSteamError *pError )
{
    if (pbDownloaded) *pbDownloaded = 0;

    if (0 == uAppId || UINT32_MAX == uAppId) {
        set_bad_err(pError);
        return 0;
    }

    if (pbDownloaded) *pbDownloaded = 1;
    set_ok_err(pError);
    return 1;
}
