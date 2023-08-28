// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.
#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineDelegateMacros.h"
#include "OnlineSubsystemAccelByte.h"
#include "OnlineUserCacheAccelByte.h"
#include "Models/AccelByteGroupModels.h"
#include "Interfaces/OnlineGroupsInterface.h"

class FAccelByteGroupsInfo;

typedef TSharedRef<FAccelByteGroupsInfo> FAccelByteGroupsInfoRef;
typedef TSharedPtr<FAccelByteGroupsInfo> FAccelByteGroupsInfoPtr;

/**
 * Combines FAccelByteModelsGroupInformation and IGroupInfo
 */
class ONLINESUBSYSTEMACCELBYTE_API FAccelByteGroupsInfo : public IGroupInfo
{
public:

	/** Constructor */
	explicit FAccelByteGroupsInfo(
		const FUniqueNetIdRef& InSenderUserId,
		const FString InNamespace,
		const FUniqueNetId& InOwnerId,
		const FDateTime InTimeCreated,
		const FDateTime InTimeLastUpdated,
		const FString InABAdminRoleId,
		const FString InABMemberRoleId,
		const FAccelByteModelsGroupInformation InABGroupInfo);

	/** Destructor */
	virtual ~FAccelByteGroupsInfo() {};

	static FAccelByteGroupsInfoRef Create(const FUniqueNetIdRef& InSenderUserId,
		const FString InNamespace,
		const FUniqueNetId& InOwnerId,
		const FAccelByteModelsGroupInformation InABGroupInfo,
		const FString AdminRoleId,
		const FString MemberRoleId);

	//~ Begin IGroupInfo overrides
	virtual FUniqueNetIdRef GetGroupId() const override;
	virtual const FString& GetNamespace() const override;
	virtual FUniqueNetIdRef GetOwner() const override;

	/* Deprecated. Use GetABDisplayInfo. */
	virtual const FGroupDisplayInfo& GetDisplayInfo() const override;
	virtual FString GetAdminRoleId();
	virtual FString GetMemberRoleId();

	virtual uint32 GetSize() const override;
	virtual const FDateTime& GetCreatedAt() const override;
	virtual const FDateTime& GetLastUpdated() const override;
	//~ End IGroupInfo overrides

	virtual const FAccelByteModelsGroupInformation& GetABDisplayInfo() const;

PACKAGE_SCOPE:
	//~ Begin IGroupInfo variables
	FUniqueNetIdRef SenderUserId;
	FString Namespace{};
	TSharedPtr<const FUniqueNetIdAccelByteUser> OwnerId = nullptr;
	FDateTime TimeCreated{};
	FDateTime TimeLastUpdated{};
	//~ End IGroupInfo variables

	FAccelByteModelsGroupInformation ABGroupInfo{};
	FString ABAdminRoleId{};
	FString ABMemberRoleId{};
	FAccelByteModelsGetMemberRequestsListResponse QueryGroupInviteReults{};
};

/**
 * Implementation of Groups service from AccelByte services
 */
class ONLINESUBSYSTEMACCELBYTE_API FOnlineGroupsAccelByte : public IOnlineGroups, public TSharedFromThis<FOnlineGroupsAccelByte, ESPMode::ThreadSafe>
{
PACKAGE_SCOPE:
	/** Constructor that is invoked by the Subsystem instance to create a user cloud instance */
	FOnlineGroupsAccelByte(FOnlineSubsystemAccelByte* InSubsystem)
		: AccelByteSubsystem(InSubsystem)
	{}

public:

	virtual ~FOnlineGroupsAccelByte() override = default;

	static bool GetFromSubsystem(const IOnlineSubsystem* Subsystem, TSharedPtr<FOnlineGroupsAccelByte, ESPMode::ThreadSafe>& OutInterfaceInstance);

	virtual void CreateGroup(const FUniqueNetId& UserIdCreatingGroup, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void CreateGroup(const FUniqueNetId& ContextUserId, const FGroupDisplayInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void JoinGroup(const FUniqueNetId& UserIdJoiningGroup, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void JoinGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void LeaveGroup(const FUniqueNetId& UserIdLeavingGroup, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void LeaveGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void AcceptInvite(const FUniqueNetId& UserIdAcceptedIntoGroup, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void AcceptInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void DeclineInvite(const FUniqueNetId& UserIdToDecline, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void DeclineInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void CancelInvite(const FUniqueNetId& AdminUserId, const FString& UserIdToCancel, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void CancelInvite(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void AcceptUser(const int32 AdminLocalUserNum, const FUniqueNetId& UserIdToAccept, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void AcceptUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void DeclineUser(const int32 AdminLocalUserNum, const FUniqueNetId& UserIdToDecline, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void DeclineUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void InviteUser(const FUniqueNetId& InviterUserId, const FUniqueNetId& InvitedUserId, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void InviteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, bool bAllowBlocked, const FOnGroupsRequestCompleted& OnCompleted) override;
	virtual void InviteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override { InviteUser(ContextUserId, GroupId, UserId, false, OnCompleted); }

	virtual void RemoveUser(const int32 AdminLocalUserNum, const FUniqueNetId& MemberUserIdToKick, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void RemoveUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void PromoteUser(const int32 AdminLocalUserNum, const FUniqueNetId& MemberUserIdToPromote, const FAccelByteGroupsInfo& InGroupInfo, const FString& MemberRoleId, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void PromoteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void DemoteUser(const int32 AdminLocalUserNum, const FUniqueNetId& MemberUserIdToDemote, const FAccelByteGroupsInfo& InGroupInfo, const FString& MemberRoleId, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void DemoteUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void QueryGroupInvites(const FUniqueNetId& ContextUserId, const FAccelByteModelsLimitOffsetRequest& AccelByteModelsLimitOffsetRequest, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void QueryGroupInvites(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;

	virtual void DeleteGroup(const int32 AdminLocalUserNum, const FAccelByteGroupsInfo& InGroupInfo, const FOnGroupsRequestCompleted& OnCompleted);
	virtual void DeleteGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override;

	// Helper Functions
	bool IsGroupValid() const;

	void GetCurrentGroupMembers(TArray<FAccelByteModelsGroupMember>& GroupMembers) const;

	FAccelByteGroupsInfoPtr GetCurrentGroupData();
	void SetCurrentGroupData(const FAccelByteGroupsInfoRef& InGroupInfo);
	void DeleteLocalGroupData();

	bool VerifyGroupInfo(const FAccelByteGroupsInfo& InGroupInfo);

protected:

	/** Hidden default constructor, the constructor that takes in a subsystem instance should be used instead. */
	FOnlineGroupsAccelByte()
		: AccelByteSubsystem(nullptr)
	{}

	/** Instance of the subsystem that created this interface */
	FOnlineSubsystemAccelByte* AccelByteSubsystem = nullptr;

	// Unused overrides
	// CreateGroup--
	virtual void FindGroups(const FUniqueNetId& ContextUserId, const FGroupSearchOptions& SearchOptions, const FOnFindGroupsCompleted& OnCompleted) override {}
	virtual void QueryGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual void QueryGroupNameExist(const FUniqueNetId& ContextUserId, const FString& GroupName, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const IGroupInfo> GetCachedGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) { return nullptr; }
	// JoinGroup--
	// LeaveGroup--
	virtual void CancelRequest(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	// AcceptInvite--
	// DeclineInvite--
	virtual void QueryGroupRoster(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const IGroupRoster> GetCachedGroupRoster(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override { return nullptr; }
	virtual void QueryUserMembership(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const IUserMembership> GetCachedUserMembership(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId) override { return nullptr; }
	virtual void QueryOutgoingApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const IApplications> GetCachedApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId) override { return nullptr; }
	virtual void QueryOutgoingInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual void QueryIncomingInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const IInvitations> GetCachedInvitations(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId) override { return nullptr; }
	virtual void UpdateGroupInfo(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FGroupDisplayInfo& GroupInfo, const FOnGroupsRequestCompleted& OnCompleted) override {}
	// AcceptUser
	// DeclineUser
	// InviteUser-FuncA
	// InviteUser-FuncB
	// CancelInvite--
	// RemoveUser--
	// PromoteUser--
	// DemoteUser--
	virtual void BlockUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual void UnblockUser(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	// QueryGroupInvites--
	virtual TSharedPtr<const IGroupInvites> GetCachedGroupInvites(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override { return nullptr; }
	virtual void QueryGroupRequests(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const IGroupRequests> GetCachedGroupRequests(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override { return nullptr; }
#if ENGINE_MAJOR_VERSION >= 5
	virtual void QueryGroupDenylist(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const IGroupDenylist> GetCachedGroupDenylist(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override { return nullptr; }
#else
	virtual void QueryGroupBlacklist(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const IGroupBlacklist> GetCachedGroupBlacklist(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId) override { return nullptr; }
#endif
	virtual void QueryIncomingApplications(const FUniqueNetId& ContextUserId, const FUniqueNetId& UserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual void QueryConfigHeadcount(const FUniqueNetId& ContextUserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual void QueryConfigMembership(const FUniqueNetId& ContextUserId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	virtual TSharedPtr<const FGroupConfigEntryInt> GetCachedConfigInt(const FString& Key) override { return nullptr; }
	virtual TSharedPtr<const FGroupConfigEntryBool> GetCachedConfigBool(const FString& Key) override { return nullptr; }
	virtual void TransferGroup(const FUniqueNetId& ContextUserId, const FUniqueNetId& GroupId, const FUniqueNetId& NewOwnerId, const FOnGroupsRequestCompleted& OnCompleted) override {}
	// DeleteGroup--
	virtual void SetNamespace(const FString& Ns) override {}
	virtual const FString& GetNamespace() const override { return CurrentGroup->Namespace; }

private:

	/** Current AccelByte Group Information*/
	FAccelByteGroupsInfoPtr CurrentGroup;
};