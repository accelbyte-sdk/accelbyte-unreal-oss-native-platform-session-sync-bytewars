// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineAsyncTaskAccelByteSendV2GameSessionInvite.h"
#include "OnlineSubsystemAccelByte.h"
#include "Core/AccelByteRegistry.h"
#include "OnlineSessionInterfaceV2AccelByte.h"
#include "Platform/OnlineAccelByteNativePlatformHandler.h"
#include "OnlinePredefinedEventInterfaceAccelByte.h"

using namespace AccelByte;

FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::FOnlineAsyncTaskAccelByteSendV2GameSessionInvite(FOnlineSubsystemAccelByte* const InABInterface, const FUniqueNetId& InLocalUserId, const FName& InSessionName, const FUniqueNetId& InRecipientId)
	// Initialize as a server task if we are running a dedicated server, as this doubles as a server task. Otherwise, use
	// no flags to indicate that it's a client task.
	: FOnlineAsyncTaskAccelByte(InABInterface, INVALID_CONTROLLERID, (IsRunningDedicatedServer()) ? ASYNC_TASK_FLAG_BIT(EAccelByteAsyncTaskFlags::ServerTask) : ASYNC_TASK_FLAG_BIT(EAccelByteAsyncTaskFlags::None))
	, SessionName(InSessionName)
	, RecipientId(FUniqueNetIdAccelByteUser::CastChecked(InRecipientId))
{
	if (!IsRunningDedicatedServer())
	{
		UserId = FUniqueNetIdAccelByteUser::CastChecked(InLocalUserId);
	}
}

void FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::Initialize()
{
	TRY_PIN_SUBSYSTEM();

	Super::Initialize();

	if (!IsRunningDedicatedServer())
	{
		AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("UserId: %s; Recipient ID: %s"), *UserId->ToString(), *RecipientId->GetAccelByteId());
	}
	else
	{
		AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("Recipient ID: %s"), *RecipientId->GetAccelByteId());
	}

	// First, check if the player is currently in a game session of given SessionName, if we're not, then we shouldn't do this
	const TSharedPtr<FOnlineSessionV2AccelByte, ESPMode::ThreadSafe> SessionInterface = StaticCastSharedPtr<FOnlineSessionV2AccelByte>(SubsystemPin->GetSessionInterface());
	AB_ASYNC_TASK_VALIDATE(SessionInterface.IsValid(), "Failed to send game session invite as our session interface is invalid!");

	FNamedOnlineSession* OnlineSession = SessionInterface->GetNamedSession(SessionName);
	AB_ASYNC_TASK_VALIDATE(OnlineSession != nullptr, "Failed to send game session invite as our local session instance is invalid!");

	// Now, once we know we are in this game session, we want to send a request to invite the player to the session
	OnSendGameSessionInviteSuccessDelegate = TDelegateUtils<FVoidHandler>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::OnSendGameSessionInviteSuccess);
	OnSendGameSessionInviteErrorDelegate = TDelegateUtils<FErrorHandler>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::OnSendGameSessionInviteError);;
	
	SessionId = OnlineSession->GetSessionIdStr();
	AB_ASYNC_TASK_VALIDATE(!SessionId.Equals(TEXT("InvalidSession"), ESearchCase::IgnoreCase), "Failed to send game session invite as our local session ID is invalid!");
	if (IsRunningDedicatedServer())
	{
		// If this is a server, send an invite through the server API client
		SERVER_API_CLIENT_CHECK_GUARD();
		ServerApiClient->ServerSession.SendGameSessionInvite(SessionId, RecipientId->GetAccelByteId(), OnSendGameSessionInviteSuccessDelegate, OnSendGameSessionInviteErrorDelegate);
		return;
	}

	// Otherwise, we need to determine the local player's current platform to see if we should attach platform info to the invite
	EAccelByteV2SessionPlatform Platform = SessionInterface->GetSessionPlatform();
	API_FULL_CHECK_GUARD(Session);
	if (Platform == EAccelByteV2SessionPlatform::Unknown)
	{
		// Platform doesn't match any of the known ones, don't attach platform info
		ApiClient->Session.SendGameSessionInvite(SessionId
			, RecipientId->GetAccelByteId()
			, OnSendGameSessionInviteSuccessDelegate
			, OnSendGameSessionInviteErrorDelegate);
	}
	else
	{
		// Platform does match one of the known ones, send with platform attached to try and get native platform ID for the player invited
		AB_ASYNC_TASK_DEFINE_SDK_DELEGATES(FOnlineAsyncTaskAccelByteSendV2GameSessionInvite, SendGameSessionInvitePlatform, THandler<FAccelByteModelsV2SessionInvitePlatformResponse>);
		ApiClient->Session.SendGameSessionInvitePlatform(SessionId
			, RecipientId->GetAccelByteId()
			, Platform
			, OnSendGameSessionInvitePlatformSuccessDelegate
			, OnSendGameSessionInvitePlatformErrorDelegate);
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::Finalize()
{
	TRY_PIN_SUBSYSTEM();

	const FOnlinePredefinedEventAccelBytePtr PredefinedEventInterface = SubsystemPin->GetPredefinedEventInterface();
	if (bWasSuccessful && PredefinedEventInterface.IsValid())
	{
		FAccelByteModelsMPV2GameSessionInvitedPayload GameSessionInvitedPayload{};
		GameSessionInvitedPayload.UserId = RecipientId->GetAccelByteId();
		GameSessionInvitedPayload.GameSessionId = SessionId;
		if (!IsRunningDedicatedServer())
		{
			PredefinedEventInterface->SendEvent(LocalUserNum, MakeShared<FAccelByteModelsMPV2GameSessionInvitedPayload>(GameSessionInvitedPayload));
		}
		else
		{
			PredefinedEventInterface->SendEvent(-1, MakeShared<FAccelByteModelsMPV2GameSessionInvitedPayload>(GameSessionInvitedPayload));
		}
	}
}

void FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::TriggerDelegates()
{
	TRY_PIN_SUBSYSTEM();

	Super::TriggerDelegates();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("bWasSuccessful: %s"), LOG_BOOL_FORMAT(bWasSuccessful));

	FOnlineSessionV2AccelBytePtr SessionInterface = nullptr;
	if (!ensureAlways(FOnlineSessionV2AccelByte::GetFromSubsystem(SubsystemPin.Get(), SessionInterface)))
	{
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to get session interface instance from online subsystem!"));
		return;
	}

	SessionInterface->TriggerOnSendSessionInviteCompleteDelegates(UserId.ToSharedRef().Get(), SessionName, bWasSuccessful, RecipientId.Get());

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::OnSendGameSessionInviteSuccess()
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::OnSendGameSessionInviteError(int32 ErrorCode, const FString& ErrorMessage)
{
	UE_LOG_AB(Warning, TEXT("Failed to invite user '%s' to game session as the request failed on the backend! Error code: %d; Error message: %s"), *RecipientId->ToDebugString(), ErrorCode, *ErrorMessage);
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
}

void FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::OnSendGameSessionInvitePlatformSuccess(const FAccelByteModelsV2SessionInvitePlatformResponse& Result)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	TRY_PIN_SUBSYSTEM();

	// Grab session interface to try and get native platform ID for this session
	FOnlineSessionV2AccelBytePtr SessionInterface = nullptr;
	if (!ensureAlways(FOnlineSessionV2AccelByte::GetFromSubsystem(SubsystemPin.Get(), SessionInterface)))
	{
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to get session interface instance from online subsystem!"));
		return;
	}

	// Check if the platform ID from the response is valid, if we have a native platform handler, and if the native session
	// ID is valid
	TSharedPtr<IOnlineAccelByteNativePlatformHandler> NativePlatformHandler = SubsystemPin.Get()->GetNativePlatformHandler();
	FString* FoundPlatformSessionId = SessionInterface->AccelByteSessionIdToNativeSessionIdMap.Find(SessionId);
	if (!Result.PlatformUserID.IsEmpty() && NativePlatformHandler.IsValid() && FoundPlatformSessionId != nullptr)
	{
		// Make a copy of the recipient ID with the proper ID in place
		FAccelByteUniqueIdComposite RecipientComposite = RecipientId->GetCompositeStructure();
		RecipientComposite.PlatformId = Result.PlatformUserID;
		FUniqueNetIdAccelByteUserRef UpdatedRecipientId = FUniqueNetIdAccelByteUser::Create(RecipientComposite);

		// Send native platform invite
		NativePlatformHandler->SendInviteToSession(UserId.ToSharedRef().Get()
			, SessionName
			, *FoundPlatformSessionId
			, UpdatedRecipientId.Get());
	}

	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteSendV2GameSessionInvite::OnSendGameSessionInvitePlatformError(int32 ErrorCode, const FString& ErrorMessage)
{
	UE_LOG_AB(Warning, TEXT("Failed to invite user '%s' to game session as the request failed on the backend! Error code: %d; Error message: %s"), *RecipientId->ToDebugString(), ErrorCode, *ErrorMessage);
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
}
