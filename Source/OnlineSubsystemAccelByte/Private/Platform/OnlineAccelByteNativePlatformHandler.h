// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once
#include "CoreMinimal.h"
#include "OnlineSubsystemAccelByte.h"

/**
 * Delegate fired when a player has joined a native platform session.
 * 
 * @param PlatformUserIdStr ID of the user that is joining the native session on the native platform
 * @param SessionId ID of the session on the AccelByte side that we need to sync to the session interface.
 * @param PartyCode Unique code assigned to a party session to allow friends to join an invite only party, will be blank if in a game session
 */
DECLARE_MULTICAST_DELEGATE_FourParams(FOnNativePlatformSessionJoined, FString /*PlatformUserIdStr*/, FString /*SessionType*/, FString /*SessionId*/, FString /*PartyCode*/)

const FString AccelByteNativeSessionIdKey = TEXT("AB_SESSION_ID");
const FString AccelByteNativeSessionPartyCodeKey = TEXT("AB_PARTY_CODE");
const FString AccelByteNativeSessionTypeKey = TEXT("AB_SESSION_TYPE");

/**
 * Interface for dealing with native platforms in the AccelByte OSS when the platform's OnlineSubsystem does not provide
 * functionality that we need to utilize. This interface is primarily designed to facilitate handling syncing AccelByte
 * sessions to other platforms.
 * 
 * To get the proper instance to use in other interfaces, call FOnlineSubsystemAccelByte::GetNativePlatformHandler.
 * 
 * This class is intended to be internal to the AccelByte OSS with no outside users.
 */
class IOnlineAccelByteNativePlatformHandler
{
public:
	/**
	 * Delegate to determine when a player has joined a native session. Session interface adds a listener to determine
	 * when we need to sync a native platform session.
	 */
	FOnNativePlatformSessionJoined OnNativePlatformSessionJoined{};

	/**
	 * Constructs a native platform handler instance associated with the subsystem passed in.
	 */
	IOnlineAccelByteNativePlatformHandler(TSharedRef<FOnlineSubsystemAccelByte, ESPMode::ThreadSafe>&& InSubsystem)
		: Subsystem(MoveTempIfPossible(InSubsystem))
	{
	}

	virtual ~IOnlineAccelByteNativePlatformHandler()
	{
	}

	/**
	 * Initialize the native platform handler. This should register callbacks for invites, check for connection strings, etc.
	 */
	virtual void Init() = 0;

	/**
	 * Clean up the native platform handler. Should unregister callbacks if needed.
	 */
	virtual void Deinit() = 0;

	/**
	 * Advance the native platform handler by a single tick.
	 */
	virtual void Tick(float DeltaTime) = 0;

	/**
	 * Join a session on the native platform using the given session ID.
	 * 
	 * @param LocalUserId ID of the user that will be joining the native platform session, this should be an AccelByte ID that we can get platform information from
	 * @param SessionName Name of the session that will be stored locally in the native OSS when the player joins, if applicable
	 * @param SessionId ID of the session on the native platform to join
	 * @return boolean that indicates whether we have started joining a native platform session
	 */
	virtual bool JoinSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId) = 0;

	/**
	 * Leave a session on the native platform using the given session ID.
	 *
	 * @param LocalUserId ID of the user that will be leaving the native platform session, this should be an AccelByte ID that we can get platform information from
	 * @param SessionName Name of the session that will be destroyed on the native OSS, if applicable
	 * @param SessionId ID of the session on the native platform to leave
	 * @return boolean that indicates whether we have started leaving a native platform session
	 */
	virtual bool LeaveSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId) = 0;

	/**
	 * Send an invite through the native platform to the session specified.
	 * 
	 * @param LocalUserId ID of the user that will be inviting another player to the native platform session, this should be an AccelByte ID that we can get platform information from
	 * @param SessionName Name of the session that we will be inviting to from the native OSS, if applicable
	 * @param SessionId ID of the session that we are inviting another player to
	 * @param InvitedId ID of the player that we are inviting to this session, this should also be an AccelByte ID with platform information
	 * @return boolean that indicates whether we have started sending an invite to a session
	 */
	virtual bool SendInviteToSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId, const FUniqueNetId& InvitedId) = 0;

protected:
	/**
	 * Shared instance of the subsystem that owns this native platform handler.
	 */
	TSharedRef<FOnlineSubsystemAccelByte, ESPMode::ThreadSafe> Subsystem;

};
