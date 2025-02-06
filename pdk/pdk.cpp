#include "pdk.h"
#include "dll/client_known_interfaces.h"

typedef void (*__cdecl PluginCall)();

void PDK::LoadPlugin(HMODULE handle)
{
    PluginCall load = (PluginCall)GetProcAddress(handle, "GBE_Load");
    load();
    PRINT_DEBUG("Loaded plugin file");
}

void PDK::UnloLoadPlugin(HMODULE handle)
{
    PluginCall load = (PluginCall)GetProcAddress(handle, "GBE_UnLoad");
    load();
    PRINT_DEBUG("Loaded plugin file");
}

int PDK::RegisterInterface(InterfaceMaker interfaceMakePtr, const char* interfaceVersion)
{
    if (interfaceMakePtr == NULL)
        return 1;
    if (interfaceVersion == NULL)
        return 1;
    if (!client_known_interfaces.count(interfaceVersion))
        return 1;
    interfaceMap.insert(std::make_pair((void*)interfaceMakePtr, interfaceVersion));
    return 0;
}

int PDK::UnRegisterInterface(InterfaceMaker interfaceMakePtr)
{
    if (interfaceMakePtr == NULL)
        return 1;

    interfaceMap.erase((void*)interfaceMakePtr);
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

int PDK::GetPDKVersion()
{
    return 1;
}
