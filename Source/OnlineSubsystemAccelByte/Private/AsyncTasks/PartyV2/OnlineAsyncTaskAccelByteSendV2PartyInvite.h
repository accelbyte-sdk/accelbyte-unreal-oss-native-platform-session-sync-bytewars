// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.
#pragma once

#include "AsyncTasks/OnlineAsyncTaskAccelByte.h"
#include "AsyncTasks/OnlineAsyncTaskAccelByteUtils.h"
#include "OnlineSubsystemAccelByteTypes.h"
#include "OnlinePartyInterfaceAccelByte.h"

/**
 * Async Task for sending an invite to a user to join a party related session
 */
class FOnlineAsyncTaskAccelByteSendV2PartyInvite
	: public FOnlineAsyncTaskAccelByte
	, public AccelByte::TSelfPtr<FOnlineAsyncTaskAccelByteSendV2PartyInvite, ESPMode::ThreadSafe>
{
public:

	FOnlineAsyncTaskAccelByteSendV2PartyInvite(FOnlineSubsystemAccelByte* const InABInterface, const FUniqueNetId& InLocalUserId, const FName& InSessionName, const FUniqueNetId& InRecipientId);

	virtual void Initialize() override;
	virtual void Finalize() override;
	virtual void TriggerDelegates() override;

protected:

	virtual const FString GetTaskName() const override
	{
		return TEXT("FOnlineAsyncTaskAccelByteSendV2PartyInvite");
	}

private:
	/** Name of the session to send an invite for */
	FName SessionName{};

	/** ID of the user that will receive the invite as an AccelByte ID */
	TSharedRef<const FUniqueNetIdAccelByteUser> RecipientId;

	/** ID of the session to send an invite for */
	FString SessionId{};

	void OnSendPartyInviteSuccess();
	void OnSendPartyInviteError(int32 ErrorCode, const FString& ErrorMessage);

	void OnSendPartyInvitePlatformSuccess(const FAccelByteModelsV2SessionInvitePlatformResponse& Result);
	void OnSendPartyInvitePlatformError(int32 ErrorCode, const FString& ErrorMessage);

};

