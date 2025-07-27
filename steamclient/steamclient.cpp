
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#ifdef _WIN64
    #define DLL_NAME "steam_api64.dll"
#else
    #define DLL_NAME "steam_api.dll"
#endif

extern "C" __declspec( dllexport )  void* __cdecl CreateInterface( const char *pName, int *pReturnCode )
{
    using fn_create_interface_t = void* (__cdecl *)(const char *);

    auto steam_api = LoadLibraryA(DLL_NAME);
    if (!steam_api) {
        if (pReturnCode) *pReturnCode = 0;
        return nullptr;
    }

    auto create_interface = (fn_create_interface_t)GetProcAddress(steam_api, "SteamInternal_CreateInterface");
    if (!create_interface) {
        if (pReturnCode) *pReturnCode = 0;
        return nullptr;
    }

    auto ptr = create_interface(pName);
    if (pReturnCode) {
        if (ptr) {
            *pReturnCode = 1;
        } else {
            *pReturnCode = 0;
        }
    }

    return ptr;
}
