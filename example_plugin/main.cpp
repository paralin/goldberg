#include "pdk.h"

#define PRINT_DEBUG_PLUGIN(a, ...) do {FILE *t = fopen("example_debug.txt", "a"); fprintf(t, "%u " a, GetCurrentThreadId(), __VA_ARGS__); fclose(t);} while (0)

#define EXPORT extern "C" __declspec( dllexport ) 

EXPORT void GBE_Load()
{
    PRINT_DEBUG_PLUGIN("GBE_Load");
    PRINT_DEBUG_PLUGIN("Version of PDK: %d", PDK::GetPDKVersion());
    // Which one should be good? idk
    //PDK::RegisterInterface(TestCreate, "STEAMAPPLIST_INTERFACE_VERSION001");
    int success = PDK::RegisterInterface(&TestCreate, "STEAMAPPLIST_INTERFACE_VERSION001");
    PRINT_DEBUG_PLUGIN("Finished registering interfaces! Is Success: %d", success);
}

EXPORT void GBE_UnLoad()
{
    PRINT_DEBUG_PLUGIN("GBE_UnLoad");
    int success = PDK::UnRegisterInterface(&TestCreate);
    PRINT_DEBUG_PLUGIN("Finished registering interfaces! Is Success: %d", success);
}


void* TestCreate(HSteamUser hSteamUser, HSteamPipe hSteamPipe)
{
    PRINT_DEBUG_PLUGIN("Test create with STEAMAPPLIST_INTERFACE_VERSION001");
    PRINT_DEBUG_PLUGIN("Args: %d %d", hSteamUser, hSteamPipe);
    return nullptr;
}