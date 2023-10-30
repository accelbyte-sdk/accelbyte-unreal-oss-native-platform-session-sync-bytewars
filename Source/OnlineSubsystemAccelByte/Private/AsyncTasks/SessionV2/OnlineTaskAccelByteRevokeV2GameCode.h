// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.
#pragma once

#include "AsyncTasks/OnlineAsyncTaskAccelByte.h"
#include "AsyncTasks/OnlineAsyncTaskAccelByteUtils.h"
#include "OnlineSubsystemAccelByteTypes.h"
#include "OnlineSessionInterfaceV2AccelByte.h"

/**
 * Revoke the party code currently associated with the given party session
 */
class FOnlineAsyncTaskAccelByteRevokeV2GameCode
	: public FOnlineAsyncTaskAccelByte
	, public AccelByte::TSelfPtr<FOnlineAsyncTaskAccelByteRevokeV2GameCode, ESPMode::ThreadSafe>
{
public:

	FOnlineAsyncTaskAccelByteRevokeV2GameCode(FOnlineSubsystemAccelByte* const InABInterface, const FUniqueNetId& InLocalUserId, const FName& InSessionName, const FOnRevokeGameCodeComplete& InDelegate);

	virtual void Initialize() override;
	virtual void Finalize() override;
	virtual void TriggerDelegates() override;

protected:

	virtual const FString GetTaskName() const override
	{
		return TEXT("FOnlineAsyncTaskAccelByteRevokeV2GameCode");
	}

private:
	/**
	 * Name of the local session that we are revoking a party code for
	 */
	FName SessionName{};

	/**
	 * Delegate fired when we finish revoking a code for the party passed in
	 */
	FOnRevokeGameCodeComplete Delegate{};

	FVoidHandler OnRevokeCodeSuccessDelegate;
	FErrorHandler OnRevokeCodeErrorDelegate;
	AB_ASYNC_TASK_DECLARE_SDK_DELEGATES(RevokeCode)

};

