// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineAsyncTaskAccelByteConnectChat.h"
#include "OnlinePredefinedEventInterfaceAccelByte.h"

#include "AsyncTasks/OnlineAsyncTaskAccelByteUtils.h"

using namespace AccelByte;

FOnlineAsyncTaskAccelByteConnectChat::FOnlineAsyncTaskAccelByteConnectChat(
	FOnlineSubsystemAccelByte* const InABInterface,
	const FUniqueNetId& InLocalUserId)
	: FOnlineAsyncTaskAccelByte(InABInterface)
{
	UserId = FUniqueNetIdAccelByteUser::CastChecked(InLocalUserId);
	ErrorStr = TEXT("");
}

void FOnlineAsyncTaskAccelByteConnectChat::Initialize()
{
	Super::Initialize();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("UserId: %s"), *UserId->ToDebugString());

	// Create delegates for successfully as well as unsuccessfully connecting to the AccelByte lobby websocket
	OnChatConnectSuccessDelegate = TDelegateUtils<AccelByte::Api::Chat::FChatConnectSuccess>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteConnectChat::OnChatConnectSuccess);
	OnChatConnectErrorDelegate = TDelegateUtils<FErrorHandler>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteConnectChat::OnChatConnectError);

	OnChatDisconnectedNotifDelegate = TDelegateUtils<AccelByte::Api::Chat::FChatDisconnectNotif>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteConnectChat::OnChatDisconnectedNotif);
	API_CLIENT_CHECK_GUARD(ErrorStr);
	ApiClient->Chat.SetDisconnectNotifDelegate(OnChatDisconnectedNotifDelegate);

	// Send off a request to connect to the lobby websocket, as well as connect our delegates for doing so
	ApiClient->Chat.SetConnectSuccessDelegate(OnChatConnectSuccessDelegate);
	ApiClient->Chat.SetConnectFailedDelegate(OnChatConnectErrorDelegate);
	ApiClient->Chat.Connect();

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteConnectChat::Finalize()
{
	const FOnlinePredefinedEventAccelBytePtr PredefinedEventInterface = Subsystem->GetPredefinedEventInterface();
	if (PredefinedEventInterface.IsValid() && bWasSuccessful)
	{
		FAccelByteModelsChatV2ConnectedPayload ConnectedPayload{};
		ConnectedPayload.UserId = UserId.IsValid() ? UserId->GetAccelByteId() : TEXT("");
		PredefinedEventInterface->SendEvent(LocalUserNum, MakeShared<FAccelByteModelsChatV2ConnectedPayload>(ConnectedPayload));
	}
}

void FOnlineAsyncTaskAccelByteConnectChat::TriggerDelegates()
{
	Super::TriggerDelegates();
	
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("bWasSuccessful: %s"), LOG_BOOL_FORMAT(bWasSuccessful));

	const FOnlineChatAccelBytePtr ChatInterface = StaticCastSharedPtr<FOnlineChatAccelByte>(Subsystem->GetChatInterface());
	if (ChatInterface.IsValid())
	{
		ChatInterface->TriggerOnConnectChatCompleteDelegates(LocalUserNum, bWasSuccessful, *UserId.Get(), ErrorStr);
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteConnectChat::OnChatConnectSuccess()
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));
	
	// set chat delegates to interface events
	const FOnlineChatAccelBytePtr ChatInterface = StaticCastSharedPtr<FOnlineChatAccelByte>(Subsystem->GetChatInterface());
	if (ChatInterface.IsValid())
	{
		ChatInterface->RegisterChatDelegates(UserId.ToSharedRef().Get());
	}

	// Get identity interface and set connected to chat
	const FOnlineIdentityAccelBytePtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(Subsystem->GetIdentityInterface());
	if (IdentityInterface.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = IdentityInterface->GetUserAccount(LocalUserNum);

		if (UserAccount.IsValid())
		{
			const TSharedPtr<FUserOnlineAccountAccelByte> UserAccountAccelByte = StaticCastSharedPtr<FUserOnlineAccountAccelByte>(UserAccount);
			UserAccountAccelByte->SetConnectedToChat(true);
		}
	}
	
	const FOnlinePredefinedEventAccelBytePtr PredefinedEventInterface = Subsystem->GetPredefinedEventInterface();

	// set connection closed delegates to set chat connected false when connection closed is triggered
	ChatInterface->AddOnChatConnectionClosedDelegate_Handle(LocalUserNum, FOnChatConnectionClosedDelegate::CreateLambda([this](int32 UserNum, int32 StatusCode, const FString Reason, bool bWasClean)
	{
		if (UserNum == LocalUserNum)
		{
			OnChatConnectionClosed(UserNum, Reason, bWasClean, StatusCode);
		}
	}));

	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);
	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteConnectChat::OnChatConnectError(int32 ErrorCode, const FString& ErrorMessage)
{
	ErrorStr = TEXT("login-failed-chat-connect-error");
	UE_LOG_AB(Warning, TEXT("Failed to connect to the AccelByte chat websocket! Error Code: %d; Error Message: %s"), ErrorCode, *ErrorMessage);
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
}

void FOnlineAsyncTaskAccelByteConnectChat::OnChatDisconnectedNotif(const FAccelByteModelsChatDisconnectNotif& Result)
{
	// Update identity interface chat flag
	const TSharedPtr<FOnlineIdentityAccelByte, ESPMode::ThreadSafe> IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(Subsystem->GetIdentityInterface());
	if (IdentityInterface.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = IdentityInterface->GetUserAccount(LocalUserNum);

		if (UserAccount.IsValid())
		{
			const TSharedPtr<FUserOnlineAccountAccelByte> UserAccountAccelByte = StaticCastSharedPtr<FUserOnlineAccountAccelByte>(UserAccount);
			UserAccountAccelByte->SetConnectedToChat(false);
		}
	}

	ErrorStr = TEXT("login-failed-chat-connect-error");

	UnbindDelegates();

	UE_LOG_AB(Warning, TEXT("Chat disconnected. Reason '%s'"), *Result.Message);
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
}

void FOnlineAsyncTaskAccelByteConnectChat::OnChatConnectionClosed(int32 InLocalUserNum, const FString& Reason, bool WasClean, int32 StatusCode)
{
	UE_LOG_AB(Warning, TEXT("Lobby connection closed. Reason '%s' Code : '%d'"), *Reason, StatusCode);

	const FOnlineIdentityAccelBytePtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(Subsystem->GetIdentityInterface());
	if (!IdentityInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Error due to either IdentityInterface or ChatInteface is invalid"));
		return;
	}

	const FUniqueNetIdPtr LocalUserId = IdentityInterface->GetUniquePlayerId(InLocalUserNum);
	if (!LocalUserId.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Error due to invalid Local User"));
		return;
	}

	TSharedPtr<FUserOnlineAccount> UserAccount = IdentityInterface->GetUserAccount(LocalUserId);
	if (!UserAccount.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Error due to invalid User Account"));
		return;
	}

	const TSharedPtr<FUserOnlineAccountAccelByte> UserAccountAccelByte = StaticCastSharedPtr<FUserOnlineAccountAccelByte>(UserAccount);
	UserAccountAccelByte->SetConnectedToChat(false);

	const FOnlinePredefinedEventAccelBytePtr PredefinedEventInterface = Subsystem->GetPredefinedEventInterface();
	if (PredefinedEventInterface.IsValid())
	{
		FUniqueNetIdAccelByteUserPtr AccelByteUserId = FUniqueNetIdAccelByteUser::TryCast(LocalUserId.ToSharedRef());
		if (AccelByteUserId.IsValid())
		{
			FAccelByteModelsChatV2DisconnectedPayload DisconnectedPayload{};
			DisconnectedPayload.UserId = AccelByteUserId->GetAccelByteId();
			DisconnectedPayload.StatusCode = StatusCode;
			PredefinedEventInterface->SendEvent(InLocalUserNum, MakeShared<FAccelByteModelsChatV2DisconnectedPayload>(DisconnectedPayload));
		}
	}
}

void FOnlineAsyncTaskAccelByteConnectChat::UnbindDelegates()
{
	OnChatConnectSuccessDelegate.Unbind();
	OnChatConnectErrorDelegate.Unbind();
	OnChatDisconnectedNotifDelegate.Unbind();
}
