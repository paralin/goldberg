#ifndef PDK_INCLUDE_H
#define PDK_INCLUDE_H

#include "dll/base.h"
#include "dll/client_known_interfaces.h"

typedef void* (__cdecl* InterfaceMaker)(HSteamUser hSteamUser, HSteamPipe hSteamPipe);

class PDK
{
    static inline std::map<void* /* interfaceMaker */, const char* /* interfaceVersion */> interfaceMap;
public:

    static int RegisterInterface(InterfaceMaker interfaceMakePtr, const char* interfaceVersion);
    static int UnRegisterInterface(InterfaceMaker interfaceMakePtr);
    static void* MakeInterface(HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char* interfaceVersion);
};


#endif //PDK_INCLUDE_H
