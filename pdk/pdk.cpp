#include "pdk.h"

/// <summary>
/// Registering from the Maker
/// </summary>
/// <param name="interfaceMakePtr"></param>
/// <param name="interfaceVersion"></param>
/// <returns>0 for success, 1 if failed</returns>
int PDK::RegisterInterface(InterfaceMaker interfaceMakePtr, const char* interfaceVersion)
{
    if (interfaceMakePtr == NULL)
        return 1;
    if (interfaceVersion == NULL)
        return 1;
    if (!client_known_interfaces.count(interfaceVersion))
        return 1;
    interfaceMap.insert(std::make_pair(interfaceMakePtr, interfaceVersion));
    return 0;
}

/// <summary>
/// Unregistering from the Maker
/// </summary>
/// <param name="interfaceMakePtr"></param>
/// <returns>0 for success, 1 if failed</returns>
int PDK::UnRegisterInterface(InterfaceMaker interfaceMakePtr)
{
    if (interfaceMakePtr == NULL)
        return 1;

    interfaceMap.erase(interfaceMakePtr);
    return 0;
}

void* PDK::MakeInterface(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char* interfaceVersion)
{
    for (const auto& [key, value] : interfaceMap)
    {
        if (strstr(interfaceVersion, value) == 0)
        {
            auto maker = (InterfaceMaker)key;
            return maker(hSteamUser, hSteamPipe);
        }
    }
    return nullptr;
}


void* TestCreate(HSteamUser hSteamUser, HSteamPipe hSteamPipe)
{
    return nullptr;
}

void Register()
{
    // Which one should be good? idk
    //PDK::RegisterInterface(TestCreate, "STEAMAPPLIST_INTERFACE_VERSION001");
    //PDK::RegisterInterface(&TestCreate, "STEAMAPPLIST_INTERFACE_VERSION001");
}
