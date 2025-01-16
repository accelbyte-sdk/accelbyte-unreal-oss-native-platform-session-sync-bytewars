// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "OnlineSubsystemAccelByte.h"
#include "AsyncTasks/OnlineAsyncTaskAccelByte.h"
#include "AsyncTasks/OnlineAsyncTaskAccelByteUtils.h"
#include "OnlineGroupsInterfaceAccelByte.h"

class FOnlineAsyncTaskAccelByteGroupsUpdateGroupCustomRule : public FOnlineAsyncTaskAccelByte, public AccelByte::TSelfPtr<FOnlineAsyncTaskAccelByteGroupsUpdateGroupCustomRule, ESPMode::ThreadSafe>
{
public:
	FOnlineAsyncTaskAccelByteGroupsUpdateGroupCustomRule(
		FOnlineSubsystemAccelByte* const InABInterface,
		const int32& GroupAdmin,
		const FUniqueNetId& InAdminUserId,
		const FString& InGroupId,
		const FAccelByteModelsUpdateCustomRulesRequest& InRequestContent,
		const FOnGroupsRequestCompleted& InDelegate);

	virtual void Initialize() override;
	virtual void TriggerDelegates() override;
	virtual void Finalize() override;

protected:
	virtual const FString GetTaskName() const override
	{
		return TEXT("FOnlineAsyncTaskAccelByteGroupsUpdateGroupCustomRule");
	}

private:
	void OnUpdateGroupCustomRuleSuccess(const FAccelByteModelsGroupInformation& Result);
	THandler<FAccelByteModelsGroupInformation> OnSuccessDelegate;

	void OnUpdateGroupCustomRuleError(int32 ErrorCode, const FString& ErrorMessage);
	FErrorHandler OnErrorDelegate;

	/* Incoming User ID Storage Variable */
	TSharedRef<const FUniqueNetIdAccelByteUser> AdminMemberId;

	/* Incoming Group Id Storage Variable */
	FString GroupId;

	/* Temporary storage of the return group information to be sent back to the requester */
	FAccelByteModelsGroupInformation AccelByteModelsGroupInformation;

	/* Requested data of the custom rule to update or create */
	FAccelByteModelsUpdateCustomRulesRequest RequestedContent;

	/* Generic delegate used to return Success or Failure status */
	FOnGroupsRequestCompleted Delegate;

	/* Used by Delegate to return Success or Failure status */
	FUniqueNetIdAccelByteResourcePtr UniqueNetIdAccelByteResource;

	/* Success or Failure status code */
	int32 httpStatus;

	/* Error message upon failure to perform requested action */
	FString ErrorString{};
};
