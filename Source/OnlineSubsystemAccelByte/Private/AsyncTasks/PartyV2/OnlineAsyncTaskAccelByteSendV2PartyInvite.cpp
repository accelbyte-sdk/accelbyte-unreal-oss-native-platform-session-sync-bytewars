// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineAsyncTaskAccelByteSendV2PartyInvite.h"
#include "OnlineSubsystemAccelByte.h"
#include "Core/AccelByteRegistry.h"
#include "OnlineSessionInterfaceV2AccelByte.h"
#include "Platform/OnlineAccelByteNativePlatformHandler.h"
#include "OnlinePredefinedEventInterfaceAccelByte.h"

FOnlineAsyncTaskAccelByteSendV2PartyInvite::FOnlineAsyncTaskAccelByteSendV2PartyInvite(FOnlineSubsystemAccelByte* const InABInterface, const FUniqueNetId& InLocalUserId, const FName& InSessionName, const FUniqueNetId& InRecipientId)
	: FOnlineAsyncTaskAccelByte(InABInterface)
	, SessionName(InSessionName)
	, RecipientId(FUniqueNetIdAccelByteUser::CastChecked(InRecipientId))
{
	UserId = FUniqueNetIdAccelByteUser::CastChecked(InLocalUserId);
}

void FOnlineAsyncTaskAccelByteSendV2PartyInvite::Initialize()
{
	TRY_PIN_SUBSYSTEM();

	Super::Initialize();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("UserId: %s; Recipient ID: %s"), *UserId->ToString(), *RecipientId->GetAccelByteId());

	// First, check if the player is currently in a party session of given SessionName, if we're not, then we shouldn't do this
	const TSharedPtr<FOnlineSessionV2AccelByte, ESPMode::ThreadSafe> SessionInterface = StaticCastSharedPtr<FOnlineSessionV2AccelByte>(SubsystemPin->GetSessionInterface());
	AB_ASYNC_TASK_VALIDATE(SessionInterface.IsValid(), "Failed to send invite to party session as our session interface is invalid!");
	check(SessionInterface.IsValid());

	FNamedOnlineSession* OnlineSession = SessionInterface->GetNamedSession(SessionName);
	AB_ASYNC_TASK_VALIDATE(OnlineSession != nullptr, "Failed to send invite to party session as our local session instance is invalid!");

	SessionId = OnlineSession->GetSessionIdStr();
	AB_ASYNC_TASK_VALIDATE(!SessionId.Equals(TEXT("InvalidSession"), ESearchCase::IgnoreCase), "Failed to send invite to party session as our local session ID is not valid!");

	// Now, once we know we are in this party, we want to send a request to invite the player to the party, sending with platform type if known
	EAccelByteV2SessionPlatform Platform = SessionInterface->GetSessionPlatform();
	API_FULL_CHECK_GUARD(Session);
	if (Platform != EAccelByteV2SessionPlatform::Unknown)
	{
		AB_ASYNC_TASK_DEFINE_SDK_DELEGATES(FOnlineAsyncTaskAccelByteSendV2PartyInvite, SendPartyInvitePlatform, THandler<FAccelByteModelsV2SessionInvitePlatformResponse>);
		ApiClient->Session.SendPartyInvitePlatform(SessionId
			, RecipientId->GetAccelByteId()
			, Platform
			, OnSendPartyInvitePlatformSuccessDelegate
			, OnSendPartyInvitePlatformErrorDelegate);
	}
	else
	{
		AB_ASYNC_TASK_DEFINE_SDK_DELEGATES(FOnlineAsyncTaskAccelByteSendV2PartyInvite, SendPartyInvite, FVoidHandler);
		ApiClient->Session.SendPartyInvite(SessionId
			, RecipientId->GetAccelByteId()
			, OnSendPartyInviteSuccessDelegate
			, OnSendPartyInviteErrorDelegate);
	}

	SessionId = OnlineSession->GetSessionIdStr();
	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteSendV2PartyInvite::Finalize()
{
	TRY_PIN_SUBSYSTEM();

	const FOnlinePredefinedEventAccelBytePtr PredefinedEventInterface = SubsystemPin->GetPredefinedEventInterface();
	if (bWasSuccessful && PredefinedEventInterface.IsValid())
	{
		FAccelByteModelsMPV2PartySessionInvitedPayload PartySessionInvitedPayload{};
		PartySessionInvitedPayload.UserId = UserId->GetAccelByteId();
		PartySessionInvitedPayload.PartySessionId = SessionId;
		PredefinedEventInterface->SendEvent(LocalUserNum, MakeShared<FAccelByteModelsMPV2PartySessionInvitedPayload>(PartySessionInvitedPayload));
	}
}

void FOnlineAsyncTaskAccelByteSendV2PartyInvite::TriggerDelegates()
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

void FOnlineAsyncTaskAccelByteSendV2PartyInvite::OnSendPartyInviteSuccess()
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteSendV2PartyInvite::OnSendPartyInviteError(int32 ErrorCode, const FString& ErrorMessage)
{
	UE_LOG_AB(Warning, TEXT("Failed to invite user '%s' to party as the request failed on the backend! Error code: %d; Error message: %s"), *RecipientId->ToDebugString(), ErrorCode, *ErrorMessage);
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
}

void FOnlineAsyncTaskAccelByteSendV2PartyInvite::OnSendPartyInvitePlatformSuccess(const FAccelByteModelsV2SessionInvitePlatformResponse& Result)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	TRY_PIN_SUBSYSTEM()

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

void FOnlineAsyncTaskAccelByteSendV2PartyInvite::OnSendPartyInvitePlatformError(int32 ErrorCode, const FString& ErrorMessage)
{
	UE_LOG_AB(Warning, TEXT("Failed to invite user '%s' to party as the request failed on the backend! Error code: %d; Error message: %s"), *RecipientId->ToDebugString(), ErrorCode, *ErrorMessage);
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
}
