// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineAsyncTaskAccelByteUpdateStatsUsers.h"

#define ONLINE_ERROR_NAMESPACE "FOnlineStatsSystemAccelByte"

FOnlineAsyncTaskAccelByteUpdateStatsUsers::FOnlineAsyncTaskAccelByteUpdateStatsUsers(FOnlineSubsystemAccelByte* const InABInterface, const int32 InLocalUserNum,
	const TArray<FOnlineStatsUserUpdatedStats>& InBulkUpdateMultipleUserStatItems, const FOnUpdateMultipleUserStatItemsComplete& InDelegate)
	: FOnlineAsyncTaskAccelByte(InABInterface, true)
	, LocalUserNum(InLocalUserNum)
	, BulkUpdateMultipleUserStatItems(InBulkUpdateMultipleUserStatItems)
	, Delegate(InDelegate)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Construct FOnlineAsyncTaskAccelByteUpdateStatsUsers"));

	BulkUpdateUserStatItemsResult = TArray<FAccelByteModelsUpdateUserStatItemsResponse>{};

	for (auto const& UpdatedUserStat : BulkUpdateMultipleUserStatItems)
	{
		FAccelByteModelsUpdateUserStatItem UpdateUserStatItemWithStatCode;
		AccelByteUserId = FUniqueNetIdAccelByteUser::CastChecked(UpdatedUserStat.Account);
		for (auto const& Stat : UpdatedUserStat.Stats)
		{
			FString Key = Stat.Key;
			FOnlineStatValue Value;
			if (Stat.Value.GetValue().IsNumeric())
			{
				Value = Stat.Value.GetValue();
			}
			FOnlineStatUpdate::EOnlineStatModificationType UpdateStrategy = Stat.Value.GetModificationType();

			UpdateUserStatItemWithStatCode.StatCode = Stat.Key;
			UpdateUserStatItemWithStatCode.Value = FCString::Atof(*Value.ToString());
			UpdateUserStatItemWithStatCode.UpdateStrategy = ConvertUpdateStrategy(UpdateStrategy);
			UpdateUserStatItemWithStatCode.UserId = AccelByteUserId->GetAccelByteId();
		}
		BulkUpdateUserStatItemsRequest.Add(UpdateUserStatItemWithStatCode);

		StatsUsers.Add(UpdatedUserStat.Account);
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteUpdateStatsUsers::Initialize()
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Initialized"));
	Super::Initialize();

	const FOnlineIdentityAccelBytePtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(Subsystem->GetIdentityInterface());
	if (!IdentityInterface.IsValid())
	{
		ErrorMessage = TEXT("request-failed-bulk-update-users-stats-error");
		CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to bulk update users stats, identity interface is invalid!"));
		return;
	}

	auto LoginStatus = IdentityInterface->GetLoginStatus(LocalUserNum);
	if (LoginStatus != ELoginStatus::LoggedIn)
	{
		ErrorMessage = TEXT("request-failed-bulk-update-users-stats-error");
		CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to bulk update users stats, user not logged in!"));
		return;
	}

	OnBulkResetMultipleUserStatItemsValueSuccess = TDelegateUtils<THandler<TArray<FAccelByteModelsUpdateUserStatItemsResponse>>>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteUpdateStatsUsers::OnBulkUpdateUserStatsSuccess);
	OnError = TDelegateUtils<FErrorHandler>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteUpdateStatsUsers::OnBulkUpdateUserStatsFailed);

	if (IsRunningDedicatedServer())
	{
		FServerApiClientPtr ServerApiClient = FMultiRegistry::GetServerApiClient();
		ServerApiClient->ServerStatistic.BulkUpdateMultipleUserStatItemsValue(BulkUpdateUserStatItemsRequest, OnBulkResetMultipleUserStatItemsValueSuccess, OnError);
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteUpdateStatsUsers::Finalize()
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Finalize"));
	Super::Finalize();

	const FOnlineStatisticAccelBytePtr StatisticInterface = StaticCastSharedPtr<FOnlineStatisticAccelByte>(Subsystem->GetStatsInterface());
	if (StatisticInterface.IsValid())
	{
		for (const auto& UserStatsPair : OnlineUsersStatsPairs)
		{
			StatisticInterface->EmplaceStats(UserStatsPair);
		}
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteUpdateStatsUsers::TriggerDelegates()
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Trigger Delegates"));
	Super::TriggerDelegates();
	EOnlineErrorResult Result = ((bWasSuccessful) ? EOnlineErrorResult::Success : EOnlineErrorResult::RequestFailure);

	Delegate.ExecuteIfBound(ONLINE_ERROR(Result, ErrorCode, FText::FromString(ErrorMessage)), BulkUpdateUserStatItemsResult);
	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteUpdateStatsUsers::OnBulkUpdateUserStatsSuccess(const TArray<FAccelByteModelsUpdateUserStatItemsResponse>& Result)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("BulkUpdateUserStatItemsValue Success"));
	BulkUpdateUserStatItemsResult = Result;

	if (Result.Num() > 0)
	{
		for (int i = 0; i < Result.Num(); i++)
		{
			float NewValue;

			TSharedPtr<FJsonValue> NewValueJson = Result[i].Details.JsonObject.Get()->TryGetField("currentValue");
			if (NewValueJson.IsValid())
			{
				if (!NewValueJson->IsNull())
				{
					NewValue = NewValueJson->AsNumber();
				}
			}

			FString Key = Result[i].StatCode;
			FVariantData Value = NewValue;
			Stats.Add(Key, Value);

			OnlineUsersStatsPairs.Add(MakeShared<FOnlineStatsUserStats>(BulkUpdateMultipleUserStatItems[i].Account, Stats));
		}
	}

	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);
	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteUpdateStatsUsers::OnBulkUpdateUserStatsFailed(int32 Code, FString const& ErrMsg)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN_VERBOSITY(Error, TEXT("Code: %d; Message: %s"), Code, *ErrMsg);

	ErrorCode = FString::Printf(TEXT("%d"), Code);
	ErrorMessage = ErrMsg;

	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

EAccelByteStatisticUpdateStrategy FOnlineAsyncTaskAccelByteUpdateStatsUsers::ConvertUpdateStrategy(FOnlineStatUpdate::EOnlineStatModificationType Strategy)
{
	switch (Strategy)
	{
	case FOnlineStatUpdate::EOnlineStatModificationType::Largest:
		return EAccelByteStatisticUpdateStrategy::MAX;
	case FOnlineStatUpdate::EOnlineStatModificationType::Smallest:
		return EAccelByteStatisticUpdateStrategy::MIN;
	case FOnlineStatUpdate::EOnlineStatModificationType::Set:
		return EAccelByteStatisticUpdateStrategy::OVERRIDE;
	case FOnlineStatUpdate::EOnlineStatModificationType::Sum:
		return EAccelByteStatisticUpdateStrategy::INCREMENT;
	default:
		return EAccelByteStatisticUpdateStrategy::OVERRIDE;
	}
}
#undef ONLINE_ERROR_NAMESPACE