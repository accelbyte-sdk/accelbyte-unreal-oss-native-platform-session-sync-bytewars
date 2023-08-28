// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineAsyncTaskAccelByteGroupsAcceptUser.h"

using namespace AccelByte;

FOnlineAsyncTaskAccelByteGroupsAcceptUser::FOnlineAsyncTaskAccelByteGroupsAcceptUser(
	FOnlineSubsystemAccelByte* const InABInterface,
	const int32& GroupAdmin,
	const FUniqueNetId& GroupMemberUserId,
	const FAccelByteGroupsInfo& InGroupInfo,
	const FOnGroupsRequestCompleted& InDelegate)
	: FOnlineAsyncTaskAccelByte(InABInterface)
	, MemberId(FUniqueNetIdAccelByteUser::CastChecked(GroupMemberUserId))
	, GroupInfo(InGroupInfo)
	, Delegate(InDelegate)
{
	LocalUserNum = GroupAdmin;
}

void FOnlineAsyncTaskAccelByteGroupsAcceptUser::Initialize()
{
	Super::Initialize();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("MemberId: %s"), *MemberId->ToDebugString());

	OnSuccessDelegate = TDelegateUtils<THandler<FAccelByteModelsMemberRequestGroupResponse>>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteGroupsAcceptUser::OnAcceptUserSuccess);
	OnErrorDelegate = TDelegateUtils<FErrorHandler>::CreateThreadSafeSelfPtr(this, &FOnlineAsyncTaskAccelByteGroupsAcceptUser::OnAcceptUserError);

	ApiClient->Group.AcceptV2GroupJoinRequest(MemberId->GetAccelByteId(), GroupInfo.ABGroupInfo.GroupId, OnSuccessDelegate, OnErrorDelegate);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteGroupsAcceptUser::TriggerDelegates()
{
	Super::TriggerDelegates();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));

	FGroupsResult result(httpStatus, ErrorString, UniqueNetIdAccelByteResource);
	Delegate.ExecuteIfBound(result);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteGroupsAcceptUser::Finalize()
{
	Super::Finalize();

	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("bWasSuccessful: %s"), LOG_BOOL_FORMAT(bWasSuccessful));

	FOnlineGroupsAccelBytePtr GroupsInterface;
	if(!ensure(FOnlineGroupsAccelByte::GetFromSubsystem(Subsystem, GroupsInterface)))
	{
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to AcceptUser, groups interface instance is not valid!"));
		return;
	}

	if (bWasSuccessful == false)
		return;

	if (GroupsInterface->IsGroupValid() == false)
	{
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to AcceptUser, not in a group!"));
		return;
	}
		
	TArray<FAccelByteModelsGroupMember> CurrentGroupMembers;
	GroupsInterface->GetCurrentGroupMembers(CurrentGroupMembers);
	if (CurrentGroupMembers.Num() == 0)
	{
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to AcceptUser, group member list is empty!"));
		return;
	}

	FAccelByteGroupsInfoPtr CurrentGroupData = GroupsInterface->GetCurrentGroupData();
	if (!CurrentGroupData.IsValid())
	{
		AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to AcceptUser, current group data is invalid!"));
		return;
	}

	for (int i = 0; i < CurrentGroupData->ABGroupInfo.GroupMembers.Num(); i++)
	{
		if (CurrentGroupData->ABGroupInfo.GroupMembers[i].UserId == AccelByteModelsMemberRequestGroupResponse.UserId)
		{
			AB_OSS_ASYNC_TASK_TRACE_END_VERBOSITY(Warning, TEXT("Failed to AcceptUser, player already exists in group!"));
			break;
		}
	}

	// Setup group member
	FAccelByteModelsGroupMember AccelByteModelsGroupMember;
	AccelByteModelsGroupMember.MemberRoleId.Add(CurrentGroupData->ABMemberRoleId);
	AccelByteModelsGroupMember.UserId = AccelByteModelsMemberRequestGroupResponse.UserId;

	// Add the player to the current group
	CurrentGroupData->ABGroupInfo.GroupMembers.Add(AccelByteModelsGroupMember);

	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteGroupsAcceptUser::OnAcceptUserSuccess(const FAccelByteModelsMemberRequestGroupResponse& Result)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT("GroupId: %s"), *Result.GroupId);
	UniqueNetIdAccelByteResource = FUniqueNetIdAccelByteResource::Create(Result.GroupId);
	httpStatus = 200;// Success = 200
	AccelByteModelsMemberRequestGroupResponse = Result;
	CompleteTask(EAccelByteAsyncTaskCompleteState::Success);
	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}

void FOnlineAsyncTaskAccelByteGroupsAcceptUser::OnAcceptUserError(int32 ErrorCode, const FString& ErrorMessage)
{
	AB_OSS_ASYNC_TASK_TRACE_BEGIN(TEXT(""));
	httpStatus = ErrorCode;
	ErrorString = ErrorMessage;
	CompleteTask(EAccelByteAsyncTaskCompleteState::RequestFailed);
	AB_OSS_ASYNC_TASK_TRACE_END(TEXT(""));
}