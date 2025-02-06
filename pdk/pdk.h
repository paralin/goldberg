#ifndef PDK_INCLUDE_H
#define PDK_INCLUDE_H

#include "dll/base.h"

typedef void* (__cdecl* InterfaceMaker)(HSteamUser hSteamUser, HSteamPipe hSteamPipe);


class PDK
{
    static inline std::map<void* /* interfaceMaker */, const char* /* interfaceVersion */> interfaceMap;

public:
    static void LoadPlugin(void* handle);
    static void UnLoadPlugin(void* handle);

    /// <summary>
    /// Registering from the Maker
    /// </summary>
    /// <param name="interfaceMakePtr"></param>
    /// <param name="interfaceVersion"></param>
    /// <returns>0 for success, 1 if failed</returns>
    static int RegisterInterface(InterfaceMaker interfaceMakePtr, const char* interfaceVersion);

    /// <summary>
    /// Unregistering from the Maker
    /// </summary>
    /// <param name="interfaceMakePtr"></param>
    /// <returns>0 for success, 1 if failed</returns> 
    static int UnRegisterInterface(InterfaceMaker interfaceMakePtr);

    /// <summary>
    /// Make Registered interface
    /// </summary>
    /// <param name="hSteamUser"></param>
    /// <param name="hSteamPipe"></param>
    /// <param name="interfaceVersion"></param>
    /// <returns>nullptr if not found, a vaild pointer to the interface</returns>
    static void* MakeInterface(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char* interfaceVersion);

    /// <summary>
    /// Get PDK Version
    /// </summary>
    /// <returns></returns>
    static int GetPDKVersion();
};


#endif //PDK_INCLUDE_H
