﻿// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineAsyncTaskAccelBytePromoteV2GameSessionLeader.h"
#include "OnlinePredefinedEventInterfaceAccelByte.h"

using namespace AccelByte;

#define ONLINE_ERROR_NAMESPACE "FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader"

FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader(FOnlineSubsystemAccelByte* const InABInterface, const FUniqueNetId& InLocalUserId, const FString& InSessionId,	const FUniqueNetId& InTargetMemberId)
	: FOnlineAsyncTaskAccelByte(InABInterface)
	, SessionId(InSessionId)
	, TargetMemberId(FUniqueNetIdAccelByteUser::CastChecked(InTargetMemberId))
	, OnlineError(FOnlineErrorAccelByte::CreateError(ONLINE_ERROR_NAMESPACE, FString(), EOnlineErrorResult::Success))
{
	UserId = FUniqueNetIdAccelByteUser::CastChecked(InLocalUserId);
}

void FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::Initialize()
{
	Super::Initialize();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("UserId: %s; SessionId: %s; TargetMemberId: %s"), *UserId->GetAccelByteId(), *SessionId, *TargetMemberId->GetAccelByteId());

	const bool bIsRequestSent = PromoteGameSessionLeader();
	if (bIsRequestSent)
	{
		AB_OSS_ASYNC_TASK_TRACE_END(TEXT("Sent request to promote a member of this game session to leader."));
	}
	else
	{
		CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
		AB_OSS_ASYNC_TASK_TRACE_END(TEXT("Failed sent request to promote a member of this game session to leader due to internal error."));
	}
}

void FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::Finalize()
{
	TRY_PIN_SUBSYSTEM();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("bWasSuccessful: %s"), LOG_BOOL_FORMAT(bWasSuccessful));

	const FOnlinePredefinedEventAccelBytePtr PredefinedEventInterface = SubsystemPin->GetPredefinedEventInterface();
	if (bWasSuccessful && PredefinedEventInterface.IsValid())
	{
		FAccelByteModelsMPV2GameSessionLeaderPromotedPayload GameSessionLeaderPromotedPayload{};
		GameSessionLeaderPromotedPayload.PromotedUserId = TargetMemberId->GetAccelByteId();
		GameSessionLeaderPromotedPayload.GameSessionId = SessionId;
		PredefinedEventInterface->SendEvent(LocalUserNum, MakeShared<FAccelByteModelsMPV2GameSessionLeaderPromotedPayload>(GameSessionLeaderPromotedPayload));
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::TriggerDelegates()
{
	TRY_PIN_SUBSYSTEM();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("bWasSuccessful: %s"), LOG_BOOL_FORMAT(bWasSuccessful));

	const TSharedPtr<FOnlineSessionV2AccelByte, ESPMode::ThreadSafe> SessionInterface = StaticCastSharedPtr<FOnlineSessionV2AccelByte>(SubsystemPin->GetSessionInterface());
	if (SessionInterface.IsValid())
	{
		SessionInterface->TriggerOnPromoteGameSessionLeaderCompleteDelegates(TargetMemberId.Get(), OnlineError);
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

bool FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::PromoteGameSessionLeader()
{
	TRY_PIN_SUBSYSTEM(false)

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("LocalUserNum: %d"), LocalUserNum);

	const FOnlineSessionV2AccelBytePtr SessionInterface = StaticCastSharedPtr<FOnlineSessionV2AccelByte>(SubsystemPin->GetSessionInterface());

	if (!SessionInterface.IsValid())
	{
		OnlineError = FOnlineErrorAccelByte::CreateError(ONLINE_ERROR_NAMESPACE, TEXT("promote-game-session-leader-failed-session-interface-invalid"), EOnlineErrorResult::MissingInterface);

		return false;
	}

	OnPromoteGameSessionLeaderSuccessDelegate = TDelegateUtils<THandler<FAccelByteModelsV2GameSession>>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::OnPromoteGameSessionLeaderSuccess);
	OnPromoteGameSessionLeaderErrorDelegate = TDelegateUtils<FErrorHandler>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::OnPromoteGameSessionLeaderError);;

	if (IsRunningDedicatedServer())
	{
		FAccelByteInstancePtr AccelByteInstance = GetAccelByteInstance().Pin();
		if(!AccelByteInstance.IsValid())
		{
			AB_OSS_ASYNC_TASK_TRACE_END(TEXT("AccelByteInstance is invalid"));
			return false;
		}

		FServerApiClientPtr ServerApiClient = AccelByteInstance->GetServerApiClient();
		if(!ServerApiClient.IsValid())
		{
			AB_OSS_ASYNC_TASK_TRACE_END(TEXT("AccelByteInstance is invalid"));
			return false;
		}
		
		ServerApiClient->ServerSession.PromoteGameSessionLeader(SessionId, TargetMemberId->GetAccelByteId(), OnPromoteGameSessionLeaderSuccessDelegate, OnPromoteGameSessionLeaderErrorDelegate);
	}
	else
	{
		if (IsApiClientValid())
		{
			auto ApiClient = GetApiClientInternal();
			const auto Session = ApiClient->GetSessionApi().Pin();
			if (!Session.IsValid()) 
			{
				return false;
			}
			Session->PromoteGameSessionLeader(SessionId, TargetMemberId->GetAccelByteId(), OnPromoteGameSessionLeaderSuccessDelegate, OnPromoteGameSessionLeaderErrorDelegate);
		}
		else
		{
			return false;
		}
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT("Sent request to promote a member of this game session to leader."));

	return true;
}

void FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::OnPromoteGameSessionLeaderSuccess(
	const FAccelByteModelsV2GameSession& BackendSessionData)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	OnlineError = FOnlineErrorAccelByte::CreateError(ONLINE_ERROR_NAMESPACE, FString(), EOnlineErrorResult::Success);
	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT("Successfully promoted member of game session to leader!"));
}

void FOnlineAsyncTaskAccelBytePromoteV2GameSessionLeader::OnPromoteGameSessionLeaderError(int32 ErrorCode,
	const FString& ErrorMessage)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	OnlineError = FOnlineErrorAccelByte::CreateError(ONLINE_ERROR_NAMESPACE,
		ErrorCode, EOnlineErrorResult::RequestFailure);
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);

	AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to promote game session member '%s' to leader of game session '%s' as the call to the backend failed! Error code: %d; Error message: %s"), *TargetMemberId->GetAccelByteId(), *SessionId, ErrorCode, *ErrorMessage);
}

#undef ONLINE_ERROR_NAMESPACE
