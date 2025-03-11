// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.
#pragma once

class FOnlineSessionSearchResult;

#ifdef AB_PLAYSTATION_NATIVE_PLATFORM_PRESENT
#include "CoreMinimal.h"
#include "Platform/OnlineAccelByteNativePlatformHandler.h"
#include "PS5WebApiTypes.h"
#include "Crossgen/CrossgenWebApiTypes.h"

class FOnlineAccelBytePlayStationNativePlatformHandler : public IOnlineAccelByteNativePlatformHandler, public TSharedFromThis<FOnlineAccelBytePlayStationNativePlatformHandler>
{
public:
	FOnlineAccelBytePlayStationNativePlatformHandler(TSharedRef<FOnlineSubsystemAccelByte, ESPMode::ThreadSafe>&& InSubsystem);

	void Init() override;
	void Deinit() override;
	void Tick(float DeltaTime) override;
	bool JoinSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId) override;
	bool LeaveSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId) override;
	bool SendInviteToSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId, const FUniqueNetId& InvitedId);

private:
	/**
	 * Delegate handler for when an invite is accepted through the system UI. Will extract AccelByte session ID and
	 * party code from session info and notify to session interface that we have a session to join.
	 */
	void OnSessionInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& SessionInfo);

	/**
	 * Delegate handler for when an activity has been requested to activate. Checks if a session has been attached to
	 * this activity, and if so we will grab the AccelByte session ID and potential party code and notify.
	 */
	void OnGameActivityActivationRequested(const FUniqueNetId& LocalUserId, const FString& ActivityId, const FOnlineSessionSearchResult* SessionInfo);

	/**
	 * Common handler between session invite and game activity to get session data from PSN and retrieve AccelByte session
	 * data, notifying game code if found.
	 */
	void GetAccelByteSessionDataAndNotify(const FUniqueNetId& LocalUserId, const FString& SessionId);

	/**
	 * Reusable method to send a request to a PSN endpoint through Np libraries
	 * 
	 * @param LocalUserId ID of the user that will be sending the request
	 * @param URL URL to send request through, must be relative to NP base URL
	 * @param Method Method to use when sending the request
	 * @param ApiGroup API group that this request is apart of
	 * @param RequestBody JSON object to send with the request, pass nullptr for no body
	 * @param ResponseBody String representation of the response we got back from NP
	 * @param ErrorCode HTTP response code that we got for the request, return value of true implies 200
	 * @return boolean that is true if request was successful, false otherwise
	 */
	bool SendRequest(const FUniqueNetId& LocalUserId
		, const FString& URL
		, const FString& Method
		, TMap<FString, FString> Headers
		, ENpApiGroupCrossgen ApiGroup
		, TSharedPtr<FJsonObject> RequestBody
		, FString& ResponseBody
		, int32& ErrorCode);

};

#endif // AB_PLAYSTATION_NATIVE_PLATFORM_PRESENT
