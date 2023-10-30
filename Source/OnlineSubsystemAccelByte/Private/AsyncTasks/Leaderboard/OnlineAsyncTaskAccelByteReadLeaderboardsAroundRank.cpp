// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineAsyncTaskAccelByteReadLeaderboardsAroundRank.h"
#include "Containers/Array.h"

using namespace AccelByte;

FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank(FOnlineSubsystemAccelByte* const InABInterface,
     int InLocalUserNum,
     const FOnlineLeaderboardReadRef& InReadObject,
     int InRank,
     int InRange,
     bool InUseCycle,
     const FString& InCycleId)
	:FOnlineAsyncTaskAccelByte(InABInterface, InLocalUserNum, true)
	, LeaderboardReadRef(InReadObject)
	, bUseCycle(InUseCycle)
	, CycleId(InCycleId) 
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Construct FOnlineAsyncTaskAccelByteReadLeaderboards"));

	LeaderboardReadRef->ReadState = EOnlineAsyncTaskState::InProgress;

	Offset = InRank - InRange;
	Limit = InRange * 2;

	if(Offset < 0)
	{
		Offset = 0;
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::Initialize()
{
	Super::Initialize();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Initialized"));


	const FOnlineIdentityAccelBytePtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(Subsystem->GetIdentityInterface());
	if (!IdentityInterface.IsValid())
	{
		ErrorMessage = TEXT("request-failed-read-leaderboards-error-identity-invalid");
		CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to read leaderboards, identity interface is invalid!"));
		return;
	}

	ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(LocalUserNum);

	if (LoginStatus != ELoginStatus::LoggedIn)
	{
		ErrorMessage = TEXT("request-failed-read-leaderboards-error-user-not-logged-in");
		CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to read leaderboards, user not logged in!"));
		return;
	}

	const FString LeaderboardCode = LeaderboardReadRef->LeaderboardName.ToString();

	OnReadLeaderboardRankSuccessHandler = TDelegateUtils<THandler<FAccelByteModelsLeaderboardRankingResultV3>>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::OnReadLeaderboardRankSuccess);
	OnReadLeaderboardRankErrorHandler = TDelegateUtils<FErrorHandler>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::OnReadLeaderboardRankFailed);

	if(bUseCycle)
	{
		ApiClient->Leaderboard.GetRankingByCycle(
			LeaderboardCode,
			CycleId,
			Offset,
			Limit,
			OnReadLeaderboardRankSuccessHandler,
			OnReadLeaderboardRankErrorHandler);	
	}else
	{
		ApiClient->Leaderboard.GetRankingsV3(
			LeaderboardCode,
			Offset,
			Limit,
			OnReadLeaderboardRankSuccessHandler,
			OnReadLeaderboardRankErrorHandler);
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::Finalize()
{
	Super::Finalize();
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Finalize"));

	LeaderboardReadRef->ReadState = bWasSuccessful ? EOnlineAsyncTaskState::Done : EOnlineAsyncTaskState::Failed;

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::TriggerDelegates()
{
	Super::TriggerDelegates();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("TriggerDelegates"));

	const FOnlineLeaderboardAccelBytePtr LeaderboardsInterfacePtr = StaticCastSharedPtr<FOnlineLeaderboardAccelByte>(Subsystem->GetLeaderboardsInterface());

	if(!ensure(LeaderboardsInterfacePtr.IsValid()))
	{
		return;
	}

	LeaderboardsInterfacePtr->TriggerOnLeaderboardReadCompleteDelegates(bWasSuccessful);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::Tick()
{
	Super::Tick();
}

void FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::OnReadLeaderboardRankSuccess(FAccelByteModelsLeaderboardRankingResultV3 const& Result)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Read Leaderboards Success"));

	const FOnlineIdentityAccelBytePtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(Subsystem->GetIdentityInterface());
	const FOnlineLeaderboardAccelBytePtr LeaderboardsInterface = StaticCastSharedPtr<FOnlineLeaderboardAccelByte>(Subsystem->GetLeaderboardsInterface());
	if (!ensure(LeaderboardsInterface.IsValid()))
	{
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to save leaderboard data as our leaderboards interface is invalid!"));
		return;
	}

	int DataCount = 1;
	for(const FAccelByteModelsUserPoint& Data : Result.Data)
	{
		FUniqueNetIdAccelByteUserRef UniqueNetIdAccelByteUserRef = FUniqueNetIdAccelByteUser::Create(Data.userId);
		FUniqueNetIdRef UniqueNetIdRef = StaticCastSharedRef<const FUniqueNetId>(UniqueNetIdAccelByteUserRef);

		FOnlineStatsRow* LeaderboardRow = new (LeaderboardReadRef->Rows) FOnlineStatsRow(
			Data.userId,
			UniqueNetIdRef);

		LeaderboardRow->Rank = Offset + DataCount;
		LeaderboardRow->Columns.Add(FName("Point"), Data.point);
		DataCount++;
	}

	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);
	
	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteReadLeaderboardsAroundRank::OnReadLeaderboardRankFailed(int Code, FString const& Message)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN_VERBOSITY(Warning, TEXT("Code: %d; Message: %s"), Code, *Message);

	ErrorCode = FString::Printf(TEXT("%d"), Code);
	ErrorMessage = Message;
	bWasSuccessful = false;

	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}