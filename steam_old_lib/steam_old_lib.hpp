#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


#if !defined(SOLD_EXTRA_DEBUG)
    #if defined(DEBUG) || defined(_DEBUG)
        #define SOLD_EXTRA_DEBUG
    #endif
#endif


namespace soldlib {

    // https://github.com/SteamRE/open-steamworks/blob/master/Open%20Steamworks/SteamTypes.h
#define STEAM_MAX_PATH 255

// https://github.com/SteamRE/open-steamworks/blob/master/Open%20Steamworks/ESteamError.h
typedef enum ESteamError
{
    eSteamErrorNone = 0,
    eSteamErrorUnknown = 1,
    eSteamErrorLibraryNotInitialized = 2,
    eSteamErrorLibraryAlreadyInitialized = 3,
    eSteamErrorConfig = 4,
    eSteamErrorContentServerConnect = 5,
    eSteamErrorBadHandle = 6,
    eSteamErrorHandlesExhausted = 7,
    eSteamErrorBadArg = 8,
    eSteamErrorNotFound = 9,
    eSteamErrorRead = 10,
    eSteamErrorEOF = 11,
    eSteamErrorSeek = 12,
    eSteamErrorCannotWriteNonUserConfigFile = 13,
    eSteamErrorCacheOpen = 14,
    eSteamErrorCacheRead = 15,
    eSteamErrorCacheCorrupted = 16,
    eSteamErrorCacheWrite = 17,
    eSteamErrorCacheSession = 18,
    eSteamErrorCacheInternal = 19,
    eSteamErrorCacheBadApp = 20,
    eSteamErrorCacheVersion = 21,
    eSteamErrorCacheBadFingerPrint = 22,

    eSteamErrorNotFinishedProcessing = 23,
    eSteamErrorNothingToDo = 24,
    eSteamErrorCorruptEncryptedUserIDTicket = 25,
    eSteamErrorSocketLibraryNotInitialized = 26,
    eSteamErrorFailedToConnectToUserIDTicketValidationServer = 27,
    eSteamErrorBadProtocolVersion = 28,
    eSteamErrorReplayedUserIDTicketFromClient = 29,
    eSteamErrorReceiveResultBufferTooSmall = 30,
    eSteamErrorSendFailed = 31,
    eSteamErrorReceiveFailed = 32,
    eSteamErrorReplayedReplyFromUserIDTicketValidationServer = 33,
    eSteamErrorBadSignatureFromUserIDTicketValidationServer = 34,
    eSteamErrorValidationStalledSoAborted = 35,
    eSteamErrorInvalidUserIDTicket = 36,
    eSteamErrorClientLoginRateTooHigh = 37,
    eSteamErrorClientWasNeverValidated = 38,
    eSteamErrorInternalSendBufferTooSmall = 39,
    eSteamErrorInternalReceiveBufferTooSmall = 40,
    eSteamErrorUserTicketExpired = 41,
    eSteamErrorCDKeyAlreadyInUseOnAnotherClient = 42,

    eSteamErrorNotLoggedIn = 101,
    eSteamErrorAlreadyExists = 102,
    eSteamErrorAlreadySubscribed = 103,
    eSteamErrorNotSubscribed = 104,
    eSteamErrorAccessDenied = 105,
    eSteamErrorFailedToCreateCacheFile = 106,
    eSteamErrorCallStalledSoAborted = 107,
    eSteamErrorEngineNotRunning = 108,
    eSteamErrorEngineConnectionLost = 109,
    eSteamErrorLoginFailed = 110,
    eSteamErrorAccountPending = 111,
    eSteamErrorCacheWasMissingRetry = 112,
    eSteamErrorLocalTimeIncorrect = 113,
    eSteamErrorCacheNeedsDecryption = 114,
    eSteamErrorAccountDisabled = 115,
    eSteamErrorCacheNeedsRepair = 116,
    eSteamErrorRebootRequired = 117,

    eSteamErrorNetwork = 200,

    eSteamErrorOffline = 201,
} ESteamError;

// https://github.com/SteamRE/open-steamworks/blob/master/Open%20Steamworks/TSteamError.h
typedef enum EDetailedPlatformErrorType
{
	eNoDetailedErrorAvailable,
	eStandardCerrno,
	eWin32LastError,
	eWinSockLastError,
	eDetailedPlatformErrorCount
} EDetailedPlatformErrorType;

typedef struct TSteamError
{
	ESteamError eSteamError;
	EDetailedPlatformErrorType eDetailedErrorType;
	int nDetailedErrorCode;
	char szDesc[STEAM_MAX_PATH];
} TSteamError;

// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/common/SteamCommon.h
// https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/common/steamcommon.h
typedef enum 
{
	eSteamAccountStatusDefault			=	0x00000000,
	eSteamAccountStatusEmailVerified	=	0x00000001,
	/* Note: Mask value 0x2 is reserved for future use. (Some, but not all, public accounts already have this set.) */
	eSteamAccountDisabled				=	0x00000004
} ESteamAccountStatusBitFields;


bool patch(void *lib_hModule);
bool restore();

}
