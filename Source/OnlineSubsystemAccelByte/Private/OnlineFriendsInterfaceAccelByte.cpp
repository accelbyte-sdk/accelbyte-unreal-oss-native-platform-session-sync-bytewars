// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineFriendsInterfaceAccelByte.h"
#include "OnlineSubsystemAccelByte.h"
#include "OnlineIdentityInterfaceAccelByte.h"
#include "OnlinePresenceInterfaceAccelByte.h"
#include "Core/AccelByteMultiRegistry.h"
#include "Api/AccelByteLobbyApi.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteReadFriendsList.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteSendFriendInvite.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteAcceptFriendInvite.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteRejectFriendInvite.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteDeleteFriend.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteAddFriendToList.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteQueryBlockedPlayers.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteBlockPlayer.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteUnblockPlayer.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteGetRecentPlayer.h"
#include "AsyncTasks/Friends/OnlineAsyncTaskAccelByteSyncThirPartyFriend.h"
#include "OnlineSubsystemUtils.h"

#define ONLINE_ERROR_NAMESPACE "FOnlineFriendAccelByte"

FOnlineFriendAccelByte::FOnlineFriendAccelByte(const FString& InDisplayName, const TSharedRef<const FUniqueNetIdAccelByteUser>& InUserId, const EInviteStatus::Type& InInviteStatus)
	: DisplayName(InDisplayName)
	, UserId(InUserId)
	, InviteStatus(InInviteStatus)
{
}

bool FOnlineFriendAccelByte::SetUserAttribute(const FString& AttrName, const FString& AttrValue)
{
	const FString* FoundAttr = UserAttributesMap.Find(AttrName);
	if (FoundAttr == nullptr || *FoundAttr != AttrValue)
	{
		UserAttributesMap.Add(AttrName, AttrValue);
		return true;
	}

	return false;
}

EInviteStatus::Type FOnlineFriendAccelByte::GetInviteStatus() const
{
	return InviteStatus;
}

const FOnlineUserPresence& FOnlineFriendAccelByte::GetPresence() const
{
	return Presence;
}

TSharedRef<const FUniqueNetId> FOnlineFriendAccelByte::GetUserId() const
{
	return UserId;
}

FString FOnlineFriendAccelByte::GetRealName() const
{
	return DisplayName;
}

FString FOnlineFriendAccelByte::GetDisplayName(const FString& Platform) const
{
	return DisplayName;
}

bool FOnlineFriendAccelByte::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundValue = UserAttributesMap.Find(AttrName);
	if (FoundValue != nullptr)
	{
		OutAttrValue = *FoundValue;
		return true;
	}
	return false;
}

FOnlineBlockedPlayerAccelByte::FOnlineBlockedPlayerAccelByte(const FString& InDisplayName, const TSharedRef<const FUniqueNetIdAccelByteUser>& InUserId)
	: DisplayName(InDisplayName)
	, UserId(InUserId)
{
}

TSharedRef<const FUniqueNetId> FOnlineBlockedPlayerAccelByte::GetUserId() const
{
	return UserId;
}

FString FOnlineBlockedPlayerAccelByte::GetRealName() const
{
	return DisplayName;
}

FString FOnlineBlockedPlayerAccelByte::GetDisplayName(const FString& Platform) const
{
	return DisplayName;
}

bool FOnlineBlockedPlayerAccelByte::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundValue = UserAttributesMap.Find(AttrName);
	if (FoundValue != nullptr)
	{
		OutAttrValue = *FoundValue;
		return true;
	}
	return false;
}

TSharedRef<const FUniqueNetId> FOnlineRecentPlayerAccelByte::GetUserId() const
{
	return UserId.ToSharedRef();
}

FString FOnlineRecentPlayerAccelByte::GetRealName() const
{
	return DisplayName;
}

FString FOnlineRecentPlayerAccelByte::GetDisplayName(const FString& Platform) const
{
	return DisplayName;
}

bool FOnlineRecentPlayerAccelByte::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundValue = UserAttributesMap.Find(AttrName);
	if (FoundValue != nullptr)
	{
		OutAttrValue = *FoundValue;
		return true;
	}
	return false;
}

FDateTime FOnlineRecentPlayerAccelByte::GetLastSeen() const
{
	return LastSeen;
}

const FOnlineUserPresence& FOnlineRecentPlayerAccelByte::GetPresence() const
{
	return Presence;
}

FOnlineRecentPlayerAccelByte::FOnlineRecentPlayerAccelByte(const FAccelByteUserInfo& InData)
{
	UserId = InData.Id;
	DisplayName = InData.DisplayName;
}

FOnlineFriendsAccelByte::FOnlineFriendsAccelByte(FOnlineSubsystemAccelByte* InSubsystem)
	: AccelByteSubsystem(InSubsystem)
{
}

void FOnlineFriendsAccelByte::OnFriendRequestAcceptedNotificationReceived(const FAccelByteModelsAcceptFriendsNotif& Notification, int32 LocalUserNum)
{
	// First, we want to get our own net ID, as delegates will require it
	const IOnlineIdentityPtr IdentityInterface = AccelByteSubsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Recieved a notification for a friend request that has been accepted, but cannot act on it as the identity interface is not valid!"));
		return;
	}

	TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
	if (!UserId.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Recieved a notification for a friend request that has been accepted, but cannot act on it as the current user's ID is not valid!"));
		return;
	}

	// Create a basic composite ID for this user without platform info, we will get platform info later
	FAccelByteUniqueIdComposite FriendCompositeId;
	FriendCompositeId.Id = Notification.friendId;
	TSharedRef<const FUniqueNetIdAccelByteUser> FriendId = FUniqueNetIdAccelByteUser::Create(FriendCompositeId);

	// Next we want to check if the invite is already in our friends list, if it is, just update that invited user
	// otherwise, we need to send off an async task to get info about the friend and update the friends list from there
	TArray<TSharedPtr<FOnlineFriend>>* FoundFriendsList = LocalUserNumToFriendsMap.Find(LocalUserNum);
	if (FoundFriendsList != nullptr)
	{
		// If we have the friends list for this user, then we want to check for the friend that accepted our invite in the list
		TSharedPtr<FOnlineFriend>* FoundFriend = FoundFriendsList->FindByPredicate([&FriendId](const TSharedPtr<FOnlineFriend>& Friend) {
			return Friend.IsValid() && Friend->GetUserId().Get() == FriendId.Get();
		});

		// If we found the friend, then we want to set the status of them to be Accepted, otherwise we need to query the friend
		// info and add that friend from the async task
		if (FoundFriend != nullptr)
		{
			TSharedPtr<FOnlineFriendAccelByte> AccelByteFriend = StaticCastSharedPtr<FOnlineFriendAccelByte>(*FoundFriend);
			AccelByteFriend->SetInviteStatus(EInviteStatus::Accepted);
			// NOTE(Damar): set online status on presence, I don't think we need to query the presence.
			FOnlineUserPresence Presence;
			Presence.bIsOnline = true;
			Presence.Status.State = EOnlinePresenceState::Online;
			AccelByteFriend->SetPresence(Presence);
			TriggerOnInviteAcceptedDelegates(UserId.ToSharedRef().Get(), AccelByteFriend->GetUserId().Get());
			TriggerOnFriendsChangeDelegates(LocalUserNum);
		}
		else
		{
			AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteAddFriendToList>(AccelByteSubsystem, LocalUserNum, UserId.ToSharedRef().Get(), FriendId.Get(), EInviteStatus::Accepted);
		}
	}
	else
	{
		AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteAddFriendToList>(AccelByteSubsystem, LocalUserNum, UserId.ToSharedRef().Get(), FriendId.Get(), EInviteStatus::Accepted);
	}
}

void FOnlineFriendsAccelByte::OnFriendRequestReceivedNotificationReceived(const FAccelByteModelsRequestFriendsNotif& Notification, int32 LocalUserNum)
{
	// First, we want to get our own net ID, as delegates will require it
	const IOnlineIdentityPtr IdentityInterface = AccelByteSubsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Recieved a notification for a friend request that has been accepted, but cannot act on it as the identity interface is not valid!"));
		return;
	}

	TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
	if (!UserId.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Recieved a notification for a friend request that has been accepted, but cannot act on it as the current user's ID is not valid!"));
		return;
	}

	// Next, we assume that we don't have this user in our friends list already as they just sent us an invite, so just
	// create an async task to get data about that friend and then add them to the list afterwards, and fire off the delegates
	FAccelByteUniqueIdComposite FriendCompositeId;
	FriendCompositeId.Id = Notification.friendId;
	TSharedRef<const FUniqueNetIdAccelByteUser> FriendId = FUniqueNetIdAccelByteUser::Create(FriendCompositeId);

	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteAddFriendToList>(AccelByteSubsystem, LocalUserNum, UserId.ToSharedRef().Get(), FriendId.Get(), EInviteStatus::PendingInbound);
}

void FOnlineFriendsAccelByte::OnUnfriendNotificationReceived(const FAccelByteModelsUnfriendNotif& Notification, int32 LocalUserNum)
{
	FAccelByteUniqueIdComposite FriendCompositeId;
	FriendCompositeId.Id = Notification.friendId;
	const TSharedRef<const FUniqueNetIdAccelByteUser> FriendId = FUniqueNetIdAccelByteUser::Create(FriendCompositeId);

	// RemoveFriendFromList will check if the friend is already in the list, so it is safe to call with just the friend ID
	RemoveFriendFromList(LocalUserNum, FriendId);

	// Once we have removed the friend from the list, fire off the delegate to signal that we have been removed as a friend
	const IOnlineIdentityPtr IdentityInterface = AccelByteSubsystem->GetIdentityInterface();
	if (IdentityInterface.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
		if (UserId.IsValid())
		{
			TriggerOnFriendRemovedDelegates(UserId.ToSharedRef().Get(), FriendId.Get());
		}
	}
}

void FOnlineFriendsAccelByte::OnRejectFriendRequestNotificationReceived(const FAccelByteModelsRejectFriendsNotif& Notification, int32 LocalUserNum)
{
	FAccelByteUniqueIdComposite InviteCompositeId;
	InviteCompositeId.Id = Notification.userId;
	const TSharedRef<const FUniqueNetIdAccelByteUser> InviteeId = FUniqueNetIdAccelByteUser::Create(InviteCompositeId);

	// RemoveFriendFromList will check if the invited user is already in the list, so it is safe to call with just the invitee ID
	RemoveFriendFromList(LocalUserNum, InviteeId);

	// Once we have removed the invitee from the list, fire off the delegate to signal that our invite has been rejected
	const IOnlineIdentityPtr IdentityInterface = AccelByteSubsystem->GetIdentityInterface();
	if (IdentityInterface.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
		if (UserId.IsValid())
		{
			TriggerOnInviteRejectedDelegates(UserId.ToSharedRef().Get(), InviteeId.Get());
		}
	}
}

void FOnlineFriendsAccelByte::OnCancelFriendRequestNotificationReceived(const FAccelByteModelsCancelFriendsNotif& Notification, int32 LocalUserNum)
{
	FAccelByteUniqueIdComposite InviteCompositeId;
	InviteCompositeId.Id = Notification.userId;
	const TSharedRef<const FUniqueNetIdAccelByteUser> InviterId = FUniqueNetIdAccelByteUser::Create(InviteCompositeId);

	// RemoveFriendFromList will check if the invite we received is already in the list, so it is safe to call with just the inviter ID
	RemoveFriendFromList(LocalUserNum, InviterId);

	// Once we have removed the invite we received from the list, fire off the delegate to signal that the invite has been canceled
	const IOnlineIdentityPtr IdentityInterface = AccelByteSubsystem->GetIdentityInterface();
	if (IdentityInterface.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
		if (UserId.IsValid())
		{
			TriggerOnInviteAbortedDelegates(UserId.ToSharedRef().Get(), InviterId.Get());
		}
	}
}

void FOnlineFriendsAccelByte::OnPresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence, int32 LocalUserNum)
{
	TSharedPtr<FOnlineFriend> Friend = GetFriend(LocalUserNum, UserId, TEXT(""));
	if(Friend.IsValid())
	{
		TSharedPtr<FOnlineFriendAccelByte> AccelByteFriend = StaticCastSharedPtr<FOnlineFriendAccelByte>(Friend);
		AccelByteFriend->SetPresence(Presence.Get());
	}
}

void FOnlineFriendsAccelByte::RegisterRealTimeLobbyDelegates(int32 LocalUserNum)
{
	// Get our identity interface to retrieve the API client for this user
	const FOnlineIdentityAccelBytePtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(AccelByteSubsystem->GetIdentityInterface());
	if (!IdentityInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Failed to register real-time lobby as an identity interface instance could not be retrieved!"));
		return;
	}

	AccelByte::FApiClientPtr ApiClient = IdentityInterface->GetApiClient(LocalUserNum);
	if (!ApiClient.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Failed to register real-time lobby as an Api client could not be retrieved for user num %d!"), LocalUserNum);
		return;
	}

	// Set each delegate for the corresponding API client to be a new realtime delegate
	AccelByte::Api::Lobby::FAcceptFriendsNotif OnFriendRequestAcceptedNotificationReceivedDelegate = AccelByte::Api::Lobby::FAcceptFriendsNotif::CreateThreadSafeSP(AsShared(), &FOnlineFriendsAccelByte::OnFriendRequestAcceptedNotificationReceived, LocalUserNum);
	ApiClient->Lobby.SetOnFriendRequestAcceptedNotifDelegate(OnFriendRequestAcceptedNotificationReceivedDelegate);

	AccelByte::Api::Lobby::FRequestFriendsNotif OnFriendRequestReceivedNotificationReceivedDelegate = AccelByte::Api::Lobby::FRequestFriendsNotif::CreateThreadSafeSP(AsShared(), &FOnlineFriendsAccelByte::OnFriendRequestReceivedNotificationReceived, LocalUserNum);
	ApiClient->Lobby.SetOnIncomingRequestFriendsNotifDelegate(OnFriendRequestReceivedNotificationReceivedDelegate);

	AccelByte::Api::Lobby::FUnfriendNotif OnUnfriendNotificationReceivedDelegate = AccelByte::Api::Lobby::FUnfriendNotif::CreateThreadSafeSP(AsShared(), &FOnlineFriendsAccelByte::OnUnfriendNotificationReceived, LocalUserNum);
	ApiClient->Lobby.SetOnUnfriendNotifDelegate(OnUnfriendNotificationReceivedDelegate);

	AccelByte::Api::Lobby::FRejectFriendsNotif OnRejectFriendRequestNotificationReceivedDelegate = AccelByte::Api::Lobby::FRejectFriendsNotif::CreateThreadSafeSP(AsShared(), &FOnlineFriendsAccelByte::OnRejectFriendRequestNotificationReceived, LocalUserNum);
	ApiClient->Lobby.SetOnRejectFriendsNotifDelegate(OnRejectFriendRequestNotificationReceivedDelegate);

	AccelByte::Api::Lobby::FCancelFriendsNotif OnCancelFriendRequestNotificationReceivedDelegate = AccelByte::Api::Lobby::FCancelFriendsNotif::CreateThreadSafeSP(AsShared(), &FOnlineFriendsAccelByte::OnCancelFriendRequestNotificationReceived, LocalUserNum);
	ApiClient->Lobby.SetOnCancelFriendsNotifDelegate(OnCancelFriendRequestNotificationReceivedDelegate);

	// Update the friend presence
	const FOnlinePresenceAccelBytePtr PresenceInterface = StaticCastSharedPtr<FOnlinePresenceAccelByte>(AccelByteSubsystem->GetPresenceInterface());
	if (!PresenceInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Failed to register real-time lobby for a friend presence as an presence interface instance could not be retrieved"));
		return;
	}
	FOnPresenceReceivedDelegate OnPresenceReceivedDelegate = FOnPresenceReceivedDelegate::CreateThreadSafeSP(AsShared(), &FOnlineFriendsAccelByte::OnPresenceReceived, LocalUserNum);
	PresenceReceivedHandle = PresenceInterface->AddOnPresenceReceivedDelegate_Handle(OnPresenceReceivedDelegate);
}

void FOnlineFriendsAccelByte::AddFriendsToList(int32 LocalUserNum, const TArray<TSharedPtr<FOnlineFriend>>& NewFriends)
{
	TArray<TSharedPtr<FOnlineFriend>>* FoundFriendsList = LocalUserNumToFriendsMap.Find(LocalUserNum);
	if (FoundFriendsList != nullptr)
	{
		// Since we do not want duplicate entries for friends in the list, and this is only really called by ReadFriendsList
		// which gets the full friends list with invites already, then we just want to clear the existing array and add our
		// new friends list that we just retrieved
		FoundFriendsList->Empty(NewFriends.Num());
		FoundFriendsList->Append(NewFriends);
	}
	else
	{
		LocalUserNumToFriendsMap.Add(LocalUserNum, NewFriends);
	}
	TriggerOnFriendsChangeDelegates(LocalUserNum);
}

void FOnlineFriendsAccelByte::AddFriendToList(int32 LocalUserNum, const TSharedPtr<FOnlineFriend>& NewFriend)
{
	TArray<TSharedPtr<FOnlineFriend>>* FoundFriendsList = LocalUserNumToFriendsMap.Find(LocalUserNum);
	if (FoundFriendsList != nullptr)
	{
		// If we have a friends list already, check to see if we have a duplicate entry, if we do, just overwrite it with the
		// new entry, otherwise we want to add this friend instance to the array.
		TSharedPtr<FOnlineFriend>* FoundFriend = FoundFriendsList->FindByPredicate([&NewFriend](const TSharedPtr<FOnlineFriend>& Friend) {
			return Friend.IsValid() && Friend->GetUserId().Get() == NewFriend->GetUserId().Get();
		});

		if (FoundFriend != nullptr)
		{
			*FoundFriend = NewFriend;
		}
		else
		{
			FoundFriendsList->Add(NewFriend);
		}
	}
	else
	{
		TArray<TSharedPtr<FOnlineFriend>> NewFriendsList;
		NewFriendsList.Add(NewFriend);
		LocalUserNumToFriendsMap.Add(LocalUserNum, NewFriendsList);
	}
	TriggerOnFriendsChangeDelegates(LocalUserNum);
}

void FOnlineFriendsAccelByte::RemoveFriendFromList(int32 LocalUserNum, const TSharedRef<const FUniqueNetIdAccelByteUser>& FriendId)
{
	TArray<TSharedPtr<FOnlineFriend>>* FoundFriendsList = LocalUserNumToFriendsMap.Find(LocalUserNum);
	if (FoundFriendsList != nullptr)
	{
		int32 FoundFriendIndex = FoundFriendsList->IndexOfByPredicate([&FriendId](const TSharedPtr<FOnlineFriend>& Friend) {
			return Friend.IsValid() && Friend->GetUserId().Get() == FriendId.Get();
		});

		if (FoundFriendIndex != INDEX_NONE)
		{
			FoundFriendsList->RemoveAt(FoundFriendIndex);
		}
	}
	TriggerOnFriendsChangeDelegates(LocalUserNum);
}

void FOnlineFriendsAccelByte::AddBlockedPlayersToList(const TSharedRef<const FUniqueNetIdAccelByteUser>& UserId, const TArray<TSharedPtr<FOnlineBlockedPlayer>>& NewBlockedPlayers)
{
	// Try and get a local user index for the player first, as it is needed for the changed delegate
	const TSharedPtr<FOnlineIdentityAccelByte, ESPMode::ThreadSafe> IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(AccelByteSubsystem->GetIdentityInterface());
	int32 LocalUserNum;
	if (!IdentityInterface->GetLocalUserNum(UserId.Get(), LocalUserNum))
	{
		UE_LOG_AB(Warning, TEXT("Could not add blocked player to blocked players list as a LocalUserNum could not be retrieved for player %s!"), *UserId->ToString());
		return;
	}

	FBlockedPlayerArray* FoundBlockedPlayersList = UserIdToBlockedPlayersMap.Find(UserId);
	if (FoundBlockedPlayersList != nullptr)
	{
		// Since we do not want duplicate entries for blocked players in the list, and this is only really called by QueryBlockedPlayers
		// which gets the full blocked list already, then we just want to clear the existing array and add our new blocked
		// list that we just retrieved
		FoundBlockedPlayersList->Empty(NewBlockedPlayers.Num());
		FoundBlockedPlayersList->Append(NewBlockedPlayers);
	}
	else
	{
		UserIdToBlockedPlayersMap.Add(UserId, NewBlockedPlayers);
	}
	TriggerOnBlockListChangeDelegates(LocalUserNum, EFriendsLists::ToString(EFriendsLists::Default));
}

void FOnlineFriendsAccelByte::AddBlockedPlayerToList(int32 LocalUserNum, const TSharedPtr<FOnlineBlockedPlayer>& NewBlockedPlayer)
{
	// First, we want to get the user's ID from the identity interface using the local user num
	const IOnlineIdentityPtr IdentityInterface = AccelByteSubsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Failed to add blocked player to player %d's list as the identity interface was invalid!"), LocalUserNum);
		return;
	}

	TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
	if (!UserId.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Failed to add blocked player to player %d's list as we could not get their unique user ID!"), LocalUserNum);
		return;
	}

	// Convert the net ID from the identity interface to an AccelByte net ID for the map query
	TSharedRef<const FUniqueNetIdAccelByteUser> NetId = FUniqueNetIdAccelByteUser::CastChecked(UserId.ToSharedRef());
	FBlockedPlayerArray* FoundBlockedPlayersList = UserIdToBlockedPlayersMap.Find(NetId);
	if (FoundBlockedPlayersList != nullptr)
	{
		// If we have a blocked players list already, check to see if we have a duplicate entry, if we do, just overwrite it with the
		// new entry, otherwise we want to add this blocked player instance to the array.
		TSharedPtr<FOnlineBlockedPlayer>* FoundBlockedPlayer = FoundBlockedPlayersList->FindByPredicate([&NewBlockedPlayer](const TSharedPtr<FOnlineBlockedPlayer>& BlockedPlayer) {
			return BlockedPlayer.IsValid() && BlockedPlayer->GetUserId().Get() == NewBlockedPlayer->GetUserId().Get();
		});

		if (FoundBlockedPlayer != nullptr)
		{
			*FoundBlockedPlayer = NewBlockedPlayer;
		}
		else
		{
			FoundBlockedPlayersList->Add(NewBlockedPlayer);
		}
	}
	else
	{
		FBlockedPlayerArray NewBlockedPlayersList;
		NewBlockedPlayersList.Add(NewBlockedPlayer);
		UserIdToBlockedPlayersMap.Add(NetId, NewBlockedPlayersList);
	}
	TriggerOnBlockListChangeDelegates(LocalUserNum, EFriendsLists::ToString(EFriendsLists::Default));
}

void FOnlineFriendsAccelByte::RemoveBlockedPlayerFromList(int32 LocalUserNum, const TSharedRef<const FUniqueNetIdAccelByteUser>& PlayerId)
{
	// First, we want to get the user's ID from the identity interface using the local user num
	const IOnlineIdentityPtr IdentityInterface = AccelByteSubsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Failed to add blocked player to player %d's list as the identity interface was invalid!"), LocalUserNum);
		return;
	}

	TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
	if (!UserId.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("Failed to add blocked player to player %d's list as we could not get their unique user ID!"), LocalUserNum);
		return;
	}

	// Convert the net ID from the identity interface to an AccelByte net ID for the map query
	TSharedRef<const FUniqueNetIdAccelByteUser> NetId = FUniqueNetIdAccelByteUser::CastChecked(UserId.ToSharedRef());
	FBlockedPlayerArray* FoundBlockedPlayerList = UserIdToBlockedPlayersMap.Find(NetId);
	if (FoundBlockedPlayerList != nullptr)
	{
		int32 FoundBlockedPlayerIndex = FoundBlockedPlayerList->IndexOfByPredicate([&PlayerId](const TSharedPtr<FOnlineBlockedPlayer>& BlockedPlayer) {
			return BlockedPlayer.IsValid() && BlockedPlayer->GetUserId().Get() == PlayerId.Get();
		});

		if (FoundBlockedPlayerIndex != INDEX_NONE)
		{
			FoundBlockedPlayerList->RemoveAt(FoundBlockedPlayerIndex);
		}
	}
	TriggerOnBlockListChangeDelegates(LocalUserNum, EFriendsLists::ToString(EFriendsLists::Default));
}


bool FOnlineFriendsAccelByte::GetFromSubsystem(const IOnlineSubsystem* Subsystem, FOnlineFriendsAccelBytePtr& OutInterfaceInstance)
{
	OutInterfaceInstance = StaticCastSharedPtr<FOnlineFriendsAccelByte>(Subsystem->GetFriendsInterface());
	return OutInterfaceInstance.IsValid();
}

bool FOnlineFriendsAccelByte::GetFromWorld(const UWorld* World, FOnlineFriendsAccelBytePtr& OutInterfaceInstance)
{
	const IOnlineSubsystem* Subsystem = ::Online::GetSubsystem(World);
	if (Subsystem == nullptr)
	{
		OutInterfaceInstance = nullptr;
		return false;
	}

	return GetFromSubsystem(Subsystem, OutInterfaceInstance);
}

bool FOnlineFriendsAccelByte::ReadFriendsList(int32 LocalUserNum, const FString& ListName, const FOnReadFriendsListComplete& Delegate)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteReadFriendsList>(AccelByteSubsystem, LocalUserNum, ListName, Delegate);
	return true;
}

bool FOnlineFriendsAccelByte::DeleteFriendsList(int32 LocalUserNum, const FString& ListName, const FOnDeleteFriendsListComplete& Delegate)
{
	// This seems to be a method that operates as an async task, meaning that it needs to perform an action on the backend.
	// However, I'm not too sure what the use case of this would be? Should this delete any friends that the user has, as
	// well as any friend requests they have sent? It doesn't seem like any other OSS implements this, so I'm going to
	// leave it unimplemented. Though, if needed, the functionality as described above could be implemented...
	AccelByteSubsystem->ExecuteNextTick([LocalUserNum, ListName, Delegate]() {
		Delegate.ExecuteIfBound(LocalUserNum, false, ListName, TEXT("DeleteFriendsList is not implemented in the AccelByte online subsystem."));
	});
	return false;
}

bool FOnlineFriendsAccelByte::SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnSendInviteComplete& Delegate)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteSendFriendInvite>(AccelByteSubsystem, LocalUserNum, FriendId, ListName, Delegate);
	return true;
}

bool FOnlineFriendsAccelByte::SendInvite(int32 LocalUserNum, const FString& InFriendCode, const FString& ListName, const FOnSendInviteComplete& Delegate)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteSendFriendInvite>(AccelByteSubsystem, LocalUserNum, InFriendCode, ListName, Delegate);
	return true;
}

bool FOnlineFriendsAccelByte::IsPlayerBlocked(const FUniqueNetId& InUserId, const FUniqueNetId& InBlockedId)
{
	// Get Blocked Players
	TArray<TSharedRef<FOnlineBlockedPlayer>> BlockedPlayers;
	GetBlockedPlayers(InUserId, BlockedPlayers);

	// Check if is in blocked player list 
	for (TSharedRef<FOnlineBlockedPlayer> BlockedPlayer : BlockedPlayers)
	{
		if (BlockedPlayer->GetUserId().Get() == InBlockedId)
		{
			return true;
		}
	}

	return false;
}

bool FOnlineFriendsAccelByte::SyncThirdPartyPlatformFriend(int32 LocalUserNum, const FString& NativeFriendListName, const FString& AccelByteFriendListName)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteSyncThirPartyFriend>(AccelByteSubsystem, LocalUserNum, NativeFriendListName, AccelByteFriendListName);
	return true;
}

bool FOnlineFriendsAccelByte::AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnAcceptInviteComplete& Delegate)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteAcceptFriendInvite>(AccelByteSubsystem, LocalUserNum, FriendId, ListName, Delegate);
	return true;
}

bool FOnlineFriendsAccelByte::RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteRejectFriendInvite>(AccelByteSubsystem, LocalUserNum, FriendId, ListName);
	return true;
}

bool FOnlineFriendsAccelByte::DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteDeleteFriend>(AccelByteSubsystem, LocalUserNum, FriendId, ListName);
	return true;
}

void FOnlineFriendsAccelByte::SetFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FString& Alias, const FOnSetFriendAliasComplete& Delegate)
{
	UE_LOG_AB(Warning, TEXT("FOnlineFriendsAccelByte::SetFriendAlias is not implemented"));
	const TSharedRef<const FUniqueNetIdAccelByteUser> NetId = FUniqueNetIdAccelByteUser::CastChecked(FriendId);
	AccelByteSubsystem->ExecuteNextTick([LocalUserNum, NetId, ListName, Delegate]() {
		Delegate.ExecuteIfBound(LocalUserNum, NetId.Get(), ListName, ONLINE_ERROR(EOnlineErrorResult::NotImplemented));
	});
}

void FOnlineFriendsAccelByte::DeleteFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnDeleteFriendAliasComplete& Delegate)
{
	UE_LOG_AB(Warning, TEXT("FOnlineFriendsAccelByte::DeleteFriendAlias is not implemented"));
	const TSharedRef<const FUniqueNetIdAccelByteUser> NetId = FUniqueNetIdAccelByteUser::CastChecked(FriendId);
	AccelByteSubsystem->ExecuteNextTick([LocalUserNum, NetId, ListName, Delegate]() {
		Delegate.ExecuteIfBound(LocalUserNum, NetId.Get(), ListName, ONLINE_ERROR(EOnlineErrorResult::NotImplemented));
	});
}

void FOnlineFriendsAccelByte::AddRecentPlayers(const FUniqueNetId& UserId, const TArray<FReportPlayedWithUser>& InRecentPlayers, const FString& ListName, const FOnAddRecentPlayersComplete& InCompletionDelegate)
{
	UE_LOG_AB(Warning, TEXT("FOnlineFriendsAccelByte::AddRecentPlayers is not implemented"));
	const TSharedRef<const FUniqueNetIdAccelByteUser> NetId = FUniqueNetIdAccelByteUser::CastChecked(UserId);
	AccelByteSubsystem->ExecuteNextTick([NetId, InCompletionDelegate]() {
		InCompletionDelegate.ExecuteIfBound(NetId.Get(), ONLINE_ERROR(EOnlineErrorResult::NotImplemented));
	});
}

bool FOnlineFriendsAccelByte::QueryRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteGetRecentPlayer>(AccelByteSubsystem, UserId, Namespace);
	return true;
}

bool FOnlineFriendsAccelByte::BlockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteBlockPlayer>(AccelByteSubsystem, LocalUserNum, PlayerId);
	return true;
}

bool FOnlineFriendsAccelByte::UnblockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteUnblockPlayer>(AccelByteSubsystem, LocalUserNum, PlayerId);
	return true;
}

bool FOnlineFriendsAccelByte::QueryBlockedPlayers(const FUniqueNetId& UserId)
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteQueryBlockedPlayers>(AccelByteSubsystem, UserId);
	return true;
}

bool FOnlineFriendsAccelByte::GetFriendsList(int32 LocalUserNum, const FString& ListName, TArray<TSharedRef<FOnlineFriend>>& OutFriends)
{
	const TArray<TSharedPtr<FOnlineFriend>>* FriendsList = LocalUserNumToFriendsMap.Find(LocalUserNum);
	if (FriendsList != nullptr)
	{
		// Since OutFriends requires TSharedRefs, we want to iterate through each member in our found friends list, and
		// convert it to a TSharedRef and add that to the out array
		for (const TSharedPtr<FOnlineFriend>& Friend : *FriendsList)
		{
			// Check whether the pointer is valid before making it a shared ref, this shouldn't happen as we don't create
			// nullptr friend instances, but just in case...
			if (Friend.IsValid())
			{
				OutFriends.Add(Friend.ToSharedRef());
			}
			else
			{
				UE_LOG_AB(Warning, TEXT("Friends list for local user %d had a null instance!"), LocalUserNum);
			}
		}
		return true;
	}

	return false;
}

TSharedPtr<FOnlineFriend> FOnlineFriendsAccelByte::GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	const TArray<TSharedPtr<FOnlineFriend>>* FriendsList = LocalUserNumToFriendsMap.Find(LocalUserNum);
	if (FriendsList != nullptr)
	{
		// Try and find the individual friend with a predicate that checks whether the friend is a valid pointer, and then
		// checks if the friend's FUniqueNetId matches that of the FriendId passed in
		const TSharedPtr<FOnlineFriend>* FoundFriend = FriendsList->FindByPredicate([&FriendId](const TSharedPtr<FOnlineFriend>& Friend) {
			return Friend.IsValid() && Friend->GetUserId().Get() == FriendId;
		});

		if (FoundFriend != nullptr)
		{
			return *FoundFriend;
		}
	}

	return nullptr;
}

bool FOnlineFriendsAccelByte::IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	return GetFriend(LocalUserNum, FriendId, ListName).IsValid();
}

bool FOnlineFriendsAccelByte::GetRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace, TArray<TSharedRef<FOnlineRecentPlayer>>& OutRecentPlayers)
{
	TArray<TSharedRef<FOnlineRecentPlayerAccelByte>> RecentPlayers = RecentPlayersMap.FindRef(UserId.AsShared());
	for (TSharedRef<FOnlineRecentPlayerAccelByte> RecentPlayer : RecentPlayers) {
		OutRecentPlayers.Add(RecentPlayer);
	}
	return true;
}

bool FOnlineFriendsAccelByte::GetBlockedPlayers(const FUniqueNetId& UserId, TArray<TSharedRef<FOnlineBlockedPlayer>>& OutBlockedPlayers)
{
	const TSharedRef<const FUniqueNetIdAccelByteUser> NetId = FUniqueNetIdAccelByteUser::CastChecked(UserId);
	const FBlockedPlayerArray* BlockedPlayersList = UserIdToBlockedPlayersMap.Find(NetId);
	if (BlockedPlayersList != nullptr)
	{
		// Since OutBlockedPlayers requires TSharedRefs, we want to iterate through each member in our found blocked players
		// list, and convert it to a TSharedRef and add that to the out array
		for (const TSharedPtr<FOnlineBlockedPlayer>& BlockedPlayer : *BlockedPlayersList)
		{
			// Check whether the pointer is valid before making it a shared ref, this shouldn't happen as we don't create
			// nullptr blocked player instances, but just in case...
			if (BlockedPlayer.IsValid())
			{
				OutBlockedPlayers.Add(BlockedPlayer.ToSharedRef());
			}
			else
			{
				UE_LOG_AB(Warning, TEXT("Blocked players list for user %s had a null instance!"), *UserId.ToString());
			}
		}
		return true;
	}

	return false;
}

void FOnlineFriendsAccelByte::DumpRecentPlayers() const
{
	UE_LOG_AB(Warning, TEXT("Recent players is not implemented in the AccelByte OSS."));
}

void FOnlineFriendsAccelByte::DumpBlockedPlayers() const
{
	UE_LOG_AB(Log, TEXT("Blocked Players for each user..."));
	for (const TPair<TSharedRef<const FUniqueNetIdAccelByteUser>, TArray<TSharedPtr<FOnlineBlockedPlayer>>>& KV : UserIdToBlockedPlayersMap)
	{
		UE_LOG_AB(Log, TEXT("    Blocked Players for User %s:"), *KV.Key->ToString());
		for (const TSharedPtr<FOnlineBlockedPlayer>& BlockedPlayer : KV.Value)
		{
			if (BlockedPlayer.IsValid())
			{
				UE_LOG_AB(Log, TEXT("        Blocked player ID: %s; Blocked player display name: %s"), *BlockedPlayer->GetUserId()->ToDebugString(), *BlockedPlayer->GetDisplayName());
			}
			else
			{
				UE_LOG_AB(Log, TEXT("        invalid TSharedPtr"));
			}
		}
	}
}

#undef ONLINE_ERROR_NAMESPACE
