// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.
#pragma once

#ifdef AB_STEAM_NATIVE_PLATFORM_PRESENT
#include "CoreMinimal.h"
#include "Platform/OnlineAccelByteNativePlatformHandler.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

class FOnlineAccelByteSteamNativePlatformHandler : public IOnlineAccelByteNativePlatformHandler
{
public:
	FOnlineAccelByteSteamNativePlatformHandler(TSharedRef<FOnlineSubsystemAccelByte, ESPMode::ThreadSafe>&& InSubsystem);

	void Init() override;
	void Deinit() override;
	void Tick(float DeltaTime) override;
	bool JoinSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId) override;
	bool LeaveSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId) override;
	bool SendInviteToSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId, const FUniqueNetId& InvitedId);

private:
	/**
	 * ID of the lobby that we have joined from the Steam UI. Used to wait for a call back from LobbyDataUpdate, cleared
	 * once that callback is hit.
	 */
	CSteamID LobbyIdAwaitingData{};

	/**
	 * Whether we have processed the command line for a lobby connection string already.
	 */
	bool bHasProcessedCommandLine{false};

	/**
	 * Checks the command line for the presence of a "+connect_lobby" string. If present, we will go through the motions
	 * of grabbing lobby information, and extracting AccelByte session information.
	 */
	void ProcessLobbyCommandLine();

	/**
	 * Callback from Steam when a player attempts to join a lobby from the system UI. Will extract the AccelByte session ID
	 * and notify session interface of the join.
	 */
	STEAM_CALLBACK(FOnlineAccelByteSteamNativePlatformHandler, OnLobbyJoinRequested, GameLobbyJoinRequested_t);

	/**
	 * Callback from Steam when data about a lobby is updated. Used to get lobby from Steam invites/join in progress to
	 * get AccelByte session ID.
	 */
	STEAM_CALLBACK(FOnlineAccelByteSteamNativePlatformHandler, OnLobbyDataUpdate, LobbyDataUpdate_t);

};

#endif // AB_STEAM_NATIVE_PLATFORM_PRESENT
