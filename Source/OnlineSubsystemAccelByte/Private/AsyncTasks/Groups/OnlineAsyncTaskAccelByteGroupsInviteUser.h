// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "OnlineSubsystemAccelByte.h"
#include "AsyncTasks/OnlineAsyncTaskAccelByte.h"
#include "AsyncTasks/OnlineAsyncTaskAccelByteUtils.h"
#include "Interfaces/OnlineGroupsInterface.h"

class FOnlineAsyncTaskAccelByteGroupsInviteUser
	: public FOnlineAsyncTaskAccelByte
	, public AccelByte::TSelfPtr<FOnlineAsyncTaskAccelByteGroupsInviteUser, ESPMode::ThreadSafe>
{
public:
	FOnlineAsyncTaskAccelByteGroupsInviteUser(
		FOnlineSubsystemAccelByte* const InABInterface,
		const FUniqueNetId& InviterUserId,
		const FUniqueNetId& InvitedUserId,
		const FString& InGroupId,
		const FOnGroupsRequestCompleted& InDelegate);

	virtual void Initialize() override;
	virtual void TriggerDelegates() override;
	virtual void Finalize() override;

protected:
	virtual const FString GetTaskName() const override
	{
		return TEXT("FOnlineAsyncTaskAccelByteGroupsInviteUser");
	}

private:
	void OnInviteUserSuccess(const FAccelByteModelsMemberRequestGroupResponse& Result);
	THandler<FAccelByteModelsMemberRequestGroupResponse> OnSuccessDelegate;

	void OnInviteUserError(int32 ErrorCode, const FString& ErrorMessage);
	FErrorHandler OnErrorDelegate;

	/* Incoming User ID Storage Variable */
	TSharedRef<const FUniqueNetIdAccelByteUser> InvitedUserId;

	/* Incoming Group Id Storage Variable */
	FString GroupId;

	/* Temporary data storage to be passed back to the requesting user or group */
	FAccelByteModelsMemberRequestGroupResponse AccelByteModelsMemberRequestGroupResponse;

	/* Generic delegate used to return Success or Failure status */
	FOnGroupsRequestCompleted Delegate;

	/* Used by Delegate to return Success or Failure status */
	FUniqueNetIdAccelByteResourcePtr UniqueNetIdAccelByteResource;

	/* Success or Failure status code */
	int32 httpStatus;

	/* Error message upon failure to perform requested action */
	FString ErrorString{};
};
