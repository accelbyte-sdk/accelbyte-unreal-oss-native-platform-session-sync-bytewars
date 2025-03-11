// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#ifdef AB_STEAM_NATIVE_PLATFORM_PRESENT
#include "OnlineAccelByteSteamPlatformHandler.h"
#include "HAL/IConsoleManager.h"
#include "NetConnectionAccelByte.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "OnlineSubsystemAccelByte.h"
#include "SteamSharedModule.h"

FOnlineAccelByteSteamNativePlatformHandler::FOnlineAccelByteSteamNativePlatformHandler(TSharedRef<FOnlineSubsystemAccelByte, ESPMode::ThreadSafe>&& InSubsystem)
	: IOnlineAccelByteNativePlatformHandler(MoveTempIfPossible(InSubsystem))
{
}

void FOnlineAccelByteSteamNativePlatformHandler::Init()
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteSteamNativePlatformHandler::Init()"));

	TSharedPtr<class FSteamClientInstanceHandler> SteamClientInstance = FSteamSharedModule::Get().ObtainSteamClientInstanceHandle();
	if (!SteamClientInstance.IsValid() || !SteamClientInstance->IsInitialized())
	{
		// Only process command line if we have a Steam instance ready
		return;
	}

	ProcessLobbyCommandLine();
}

void FOnlineAccelByteSteamNativePlatformHandler::Deinit()
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteSteamNativePlatformHandler::Deinit()"));
}

void FOnlineAccelByteSteamNativePlatformHandler::Tick(float DeltaTime)
{
	if (bHasProcessedCommandLine)
	{
		// Tick is only used for delayed processing of command line for now, bail if already processed
		return;
	}

	TSharedPtr<class FSteamClientInstanceHandler> SteamClientInstance = FSteamSharedModule::Get().ObtainSteamClientInstanceHandle();
	if (!SteamClientInstance.IsValid() || !SteamClientInstance->IsInitialized())
	{
		// Only process command line if we have a Steam instance ready
		return;
	}

	ProcessLobbyCommandLine();
}

bool FOnlineAccelByteSteamNativePlatformHandler::JoinSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteSteamNativePlatformHandler::JoinSession(%s)"), *SessionId);

	// First, convert the string ID to a Steam ID
	const CSteamID LobbyId = CSteamID(FCString::Strtoui64(*SessionId, nullptr, 10));

	// Then, make a call to join the lobby
	// #TODO: For now this is just fire and forget, though we may want to log an error or retry if the call fails
	SteamMatchmaking()->JoinLobby(LobbyId);
	return true;
}

bool FOnlineAccelByteSteamNativePlatformHandler::LeaveSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteSteamNativePlatformHandler::LeaveSession(%s)"), *SessionId);

	// First, convert the string ID to a Steam ID
	const CSteamID LobbyId = CSteamID(FCString::Strtoui64(*SessionId, nullptr, 10));

	// Then, make a call to leave the lobby
	// #TODO: For now this is just fire and forget, though we may want to log an error or retry if the call fails
	SteamMatchmaking()->LeaveLobby(LobbyId);
	return true;
}

bool FOnlineAccelByteSteamNativePlatformHandler::SendInviteToSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId, const FUniqueNetId& InvitedId)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteSteamNativePlatformHandler::SendInviteToSession(%s, %s)"), *SessionId, *InvitedId.ToDebugString());

	// Convert the session ID into a Steam ID
	const CSteamID LobbyId = CSteamID(FCString::Strtoui64(*SessionId, nullptr, 10));

	// Then, convert the invited user ID to a Steam ID
	FUniqueNetIdAccelByteUserRef InvitedAccelByteId = FUniqueNetIdAccelByteUser::CastChecked(InvitedId);
	const CSteamID InvitedSteamUserId = CSteamID(FCString::Strtoui64(*InvitedAccelByteId->GetPlatformId(), nullptr, 10));

	// Finally, send the request to invite the player to the lobby
	SteamMatchmaking()->InviteUserToLobby(LobbyId, InvitedSteamUserId);
	return true;
}

void FOnlineAccelByteSteamNativePlatformHandler::ProcessLobbyCommandLine()
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteSteamNativePlatformHandler::ProcessLobbyCommandLine()"));

	// First, grab command line as a string
	const TCHAR* CommandLine = FCommandLine::Get();
	const FString CommandLineStr = FString(CommandLine);

	// Second, check for the presence of the "+connect_lobby" token
	const FString ConnectLobbyKey = TEXT("+connect_lobby");
	const int32 ConnectLobbyStart = CommandLineStr.Find(ConnectLobbyKey);
	if (ConnectLobbyStart == INDEX_NONE)
	{
		// No connect lobby string on the command line, no need to go further
		bHasProcessedCommandLine = true;
		return;
	}

	// Then, grab a substring for the lobby ID and convert to a CSteamID
	const TCHAR* ParseStr = CommandLine + ConnectLobbyStart + ConnectLobbyKey.Len();
	FString LobbyIdStr = FParse::Token(ParseStr, false);
	CSteamID LobbyId = CSteamID(FCString::Strtoui64(*LobbyIdStr, nullptr, 10));

	// Finally, request lobby data for the ID
	LobbyIdAwaitingData = LobbyId;

	bool bResult = SteamMatchmaking()->RequestLobbyData(LobbyId);
	if (!bResult)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteSteamNativePlatformHandler::ProcessLobbyCommandLine(): Failed to process lobby command line from Steam as the call to RequestLobbyData failed! LobbyId: %s"), *LobbyIdStr);
		LobbyIdAwaitingData = CSteamID();
	}

	bHasProcessedCommandLine = true;
}

void FOnlineAccelByteSteamNativePlatformHandler::OnLobbyJoinRequested(GameLobbyJoinRequested_t* JoinRequestEvent)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteSteamNativePlatformHandler::OnLobbyJoinRequested(%llu)"), JoinRequestEvent->m_steamIDLobby.ConvertToUint64());

	// Grab the ID of the lobby from the event
	CSteamID LobbyId = JoinRequestEvent->m_steamIDLobby;
	if (!LobbyId.IsLobby())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteSteamNativePlatformHandler::OnLobbyJoinRequested(%llu): Failed to process lobby join from Steam as the ID provided was not a lobby!"), LobbyId.ConvertToUint64());
		return;
	}

	LobbyIdAwaitingData = LobbyId;

	// Attempt to get information about the lobby from Steam. After which, we will read the data to grab the AccelByte
	// session ID and fire the delegate to notify the session interface.
	bool bResult = SteamMatchmaking()->RequestLobbyData(LobbyId);
	if (!bResult)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteSteamNativePlatformHandler::OnLobbyJoinRequested(%llu): Failed to process lobby join from Steam as the call to RequestLobbyData failed!"), LobbyId.ConvertToUint64());
		LobbyIdAwaitingData = CSteamID();
	}
}

void FOnlineAccelByteSteamNativePlatformHandler::OnLobbyDataUpdate(LobbyDataUpdate_t* LobbyDataUpdateEvent)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteSteamNativePlatformHandler::OnLobbyDataUpdate(%llu)"), LobbyDataUpdateEvent->m_ulSteamIDLobby);

	if (!LobbyDataUpdateEvent->m_bSuccess)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteSteamNativePlatformHandler::OnLobbyDataUpdate(%llu): Request to get lobby data failed!"), LobbyDataUpdateEvent->m_ulSteamIDLobby);
		return;
	}

	const bool bIsLobbyEvent = LobbyDataUpdateEvent->m_ulSteamIDLobby == LobbyDataUpdateEvent->m_ulSteamIDMember;
	if (!bIsLobbyEvent)
	{
		// No log so that we don't run the risk of spamming for unrelated updates
		return;
	}

	const bool bIsCorrectLobby = LobbyDataUpdateEvent->m_ulSteamIDLobby == LobbyIdAwaitingData.ConvertToUint64();
	if (!bIsCorrectLobby)
	{
		// No log so that we don't run the risk of spamming for unrelated updates
		return;
	}

	if (!LobbyDataUpdateEvent->m_bSuccess)
	{
		LobbyIdAwaitingData = CSteamID();
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteSteamNativePlatformHandler::OnLobbyDataUpdate(%llu): Failed to process lobby join from Steam as the update event was unsuccessful!"), LobbyDataUpdateEvent->m_ulSteamIDLobby);
		return;
	}

	// Extract type of AccelByte session from Steam lobby data
	FString AccelByteSessionTypeStr = FString(SteamMatchmaking()->GetLobbyData(LobbyIdAwaitingData, TCHAR_TO_ANSI(*AccelByteNativeSessionTypeKey)));
	if (AccelByteSessionTypeStr.IsEmpty())
	{
		LobbyIdAwaitingData = CSteamID();
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteSteamNativePlatformHandler::OnLobbyDataUpdate(%llu): Failed to process lobby join from Steam as we could not extract the AccelByte session type!"), LobbyDataUpdateEvent->m_ulSteamIDLobby);
		return;
	}

	// Extract ID of the AccelByte session from the Steam lobby data
	FString AccelByteSessionIdStr = FString(SteamMatchmaking()->GetLobbyData(LobbyIdAwaitingData, TCHAR_TO_ANSI(*AccelByteNativeSessionIdKey)));
	if (AccelByteSessionIdStr.IsEmpty())
	{
		LobbyIdAwaitingData = CSteamID();
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteSteamNativePlatformHandler::OnLobbyDataUpdate(%llu): Failed to process lobby join from Steam as we could not extract the AccelByte session ID!"), LobbyDataUpdateEvent->m_ulSteamIDLobby);
		return;
	}

	// Also attempt to get party code, though if this is blank it probably means that we are just not joining a party
	// session
	FString AccelBytePartyCode = FString(SteamMatchmaking()->GetLobbyData(LobbyIdAwaitingData, TCHAR_TO_ANSI(*AccelByteNativeSessionPartyCodeKey)));

	// Since Steam only supports one player at a time, and the join delegate expects a user ID, just grab the SteamID
	// of the current user and convert it to a string to pass to the delegate
	FString UserIdStr = FString::Printf(TEXT("%llu"), SteamUser()->GetSteamID().ConvertToUint64());

	OnNativePlatformSessionJoined.Broadcast(UserIdStr
		, AccelByteSessionTypeStr
		, AccelByteSessionIdStr
		, AccelBytePartyCode);
}

#endif // AB_STEAM_NATIVE_PLATFORM_PRESENT
