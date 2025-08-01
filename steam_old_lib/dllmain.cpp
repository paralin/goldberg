#include "steam_old_lib.hpp"


static bool dll_loaded = false;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  reason, LPVOID lpReserved)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        if (!soldlib::patch((void*)hModule)) {

#ifdef SOLD_EXTRA_DEBUG
            MessageBoxA(nullptr, "Failed to initialize", "Main", MB_OK | MB_ICONERROR);
#endif

            // https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain
            // "The system immediately calls your entry-point function with DLL_PROCESS_DETACH and unloads the DLL"
            return FALSE;
        }

        dll_loaded = true;
    }
    break;

    case DLL_PROCESS_DETACH: {
        if (dll_loaded) {
            soldlib::restore();
        }
    }
    break;
    }

    return TRUE;
}
