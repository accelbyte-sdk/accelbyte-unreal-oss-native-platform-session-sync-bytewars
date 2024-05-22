// Copyright (c) 2024 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineAsyncTaskAccelBytePreviewOrder.h"

#define ONLINE_ERROR_NAMESPACE "FOnlinePurchaseSystemAccelByte"

FOnlineAsyncTaskAccelBytePreviewOrder::FOnlineAsyncTaskAccelBytePreviewOrder(
	FOnlineSubsystemAccelByte* const InABSubsystem, const FUniqueNetId& InUserId, const FAccelByteModelsUserPreviewOrderRequest& InPreviewOrderRequest)
	: FOnlineAsyncTaskAccelByte(InABSubsystem)
	, ErrorCode(0)
	, PreviewOrderRequest(InPreviewOrderRequest)
{
	UserId = FUniqueNetIdAccelByteUser::CastChecked(InUserId);
}

void FOnlineAsyncTaskAccelBytePreviewOrder::Initialize()
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));
	Super::Initialize();

	const THandler<FAccelByteModelsUserPreviewOrderResponse>& OnSuccess = AccelByte::TDelegateUtils<THandler<FAccelByteModelsUserPreviewOrderResponse>>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelBytePreviewOrder::HandleSuccess);
	const FErrorHandler& OnError = AccelByte::TDelegateUtils<FErrorHandler>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelBytePreviewOrder::HandleError);
	API_CLIENT_CHECK_GUARD(ErrorMessage);
	ApiClient->Order.PreviewUserOrder(PreviewOrderRequest, OnSuccess, OnError);
	
	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelBytePreviewOrder::TriggerDelegates()
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));
	Super::TriggerDelegates();

	const FOnlinePurchaseAccelBytePtr PurchaseInterface = StaticCastSharedPtr<FOnlinePurchaseAccelByte>(Subsystem->GetPurchaseInterface());
	if (PurchaseInterface.IsValid())
	{
		const FOnlineErrorAccelByte OnlineError = bWasSuccessful ? ONLINE_ERROR_ACCELBYTE(TEXT(""), EOnlineErrorResult::Success) :
			ONLINE_ERROR_ACCELBYTE(FOnlineErrorAccelByte::PublicGetErrorKey(ErrorCode, ErrorMessage));
		PurchaseInterface->TriggerOnPreviewOrderCompleteDelegates(bWasSuccessful, PreviewOrderResponse, OnlineError);
	}

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelBytePreviewOrder::HandleSuccess(const FAccelByteModelsUserPreviewOrderResponse& Result)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	PreviewOrderResponse = Result;
	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelBytePreviewOrder::HandleError(int32 Code, const FString& Message)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	ErrorCode = Code;
	ErrorMessage = Message; 
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

#undef ONLINE_ERROR_NAMESPACE