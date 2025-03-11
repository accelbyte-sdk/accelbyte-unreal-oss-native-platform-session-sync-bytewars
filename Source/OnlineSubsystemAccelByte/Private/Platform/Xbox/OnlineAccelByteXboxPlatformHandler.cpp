// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.
#ifdef AB_XBOX_NATIVE_PLATFORM_PRESENT
#include "OnlineAccelByteXboxPlatformHandler.h"
#include "OnlineIdentityInterfaceAccelByte.h"
#include "OnlineSubsystemAccelByteTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemAccelByteInternalHelpers.h"

FOnlineAccelByteXboxNativePlatformHandler::FOnlineAccelByteXboxNativePlatformHandler(TSharedRef<FOnlineSubsystemAccelByte, ESPMode::ThreadSafe>&& InSubsystem)
	: IOnlineAccelByteNativePlatformHandler(MoveTempIfPossible(InSubsystem))
{
}

void FOnlineAccelByteXboxNativePlatformHandler::Init()
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteXboxNativePlatformHandler::Init()"));

	IOnlineSubsystem* GDKSubsystem = IOnlineSubsystem::Get(TEXT("GDK"));
	if (GDKSubsystem == nullptr)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::Init(): Failed to get GDK online subsystem instance"));
		return;
	}

	IOnlineSessionPtr SessionInterface = GDKSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::Init(): Failed to get GDK session interface instance"));
		return;
	}

	const FOnSessionUserInviteAcceptedDelegate OnNativeInviteAcceptedDelegate = FOnSessionUserInviteAcceptedDelegate::CreateSP(SharedThis(this), &FOnlineAccelByteXboxNativePlatformHandler::OnNativeInviteAccepted);
	SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(OnNativeInviteAcceptedDelegate);
}

void FOnlineAccelByteXboxNativePlatformHandler::Deinit()
{
}

void FOnlineAccelByteXboxNativePlatformHandler::Tick(float DeltaTime)
{
}

bool FOnlineAccelByteXboxNativePlatformHandler::JoinSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteXboxNativePlatformHandler::JoinSession(%s, %s, %s)"), *LocalUserId.ToDebugString(), *SessionName.ToString(), *SessionId);

	// Get GDK subsystem instance
	IOnlineSubsystem* GDKSubsystem = IOnlineSubsystem::Get(TEXT("GDK"));
	if (GDKSubsystem == nullptr)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::JoinSession(): Failed to get GDK online subsystem instance"));
		return false;
	}

	// Get GDK session interface
	IOnlineSessionPtr SessionInterface = GDKSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::JoinSession(): Failed to get GDK session interface instance"));
		return false;
	}

	// Convert string session ID to unique ID and find by ID
	FUniqueNetIdPtr SessionUniqueId = SessionInterface->CreateSessionIdFromString(SessionId);
	if (!SessionUniqueId.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::JoinSession(): Failed to convert session ID to GDK unique ID. SessionId: %s"), *SessionId);
		return false;
	}

	// Grab GDK user ID from AccelByte local ID passed in
	FUniqueNetIdAccelByteUserPtr LocalAccelByteUserId = FUniqueNetIdAccelByteUser::CastChecked(LocalUserId);
	if (!LocalAccelByteUserId->HasPlatformInformation())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::JoinSession(): Failed to grab GDK unique ID from local user ID"), *SessionId);
		return false;
	}

	const FOnSingleSessionResultCompleteDelegate OnFindGDKSessionByIdCompleteDelegate = FOnSingleSessionResultCompleteDelegate::CreateSP(SharedThis(this), &FOnlineAccelByteXboxNativePlatformHandler::OnFindGDKSessionByIdComplete, SessionName);
	SessionInterface->FindSessionById(LocalAccelByteUserId->GetPlatformUniqueId().ToSharedRef().Get(), SessionUniqueId.ToSharedRef().Get(), SessionUniqueId.ToSharedRef().Get(), TEXT(""), OnFindGDKSessionByIdCompleteDelegate);
	return true;
}

bool FOnlineAccelByteXboxNativePlatformHandler::LeaveSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteXboxNativePlatformHandler::LeaveSession(%s)"), *SessionName.ToString());

	// Get GDK subsystem instance
	IOnlineSubsystem* GDKSubsystem = IOnlineSubsystem::Get(TEXT("GDK"));
	if (GDKSubsystem == nullptr)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::LeaveSession(): Failed to get GDK online subsystem instance"));
		return false;
	}

	// Get GDK session interface
	IOnlineSessionPtr SessionInterface = GDKSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::LeaveSession(): Failed to get GDK session interface instance"));
		return false;
	}

	// Find a session with the name provided and destroy
	FNamedOnlineSession* Session = SessionInterface->GetNamedSession(SessionName);
	if (Session != nullptr)
	{
		SessionInterface->DestroySession(SessionName);
		return true;
	}

	return false;
}

bool FOnlineAccelByteXboxNativePlatformHandler::SendInviteToSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId, const FUniqueNetId& InvitedId)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteXboxNativePlatformHandler::SendInviteToSession(%s, %s, %s, %s)"), *LocalUserId.ToDebugString(), *SessionName.ToString(), *SessionId, *InvitedId.ToDebugString());

	// Get the GDK subsystem instance
	IOnlineSubsystem* GDKSubsystem = IOnlineSubsystem::Get(TEXT("GDK"));
	if (GDKSubsystem == nullptr)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::SendInviteToSession(): Failed to get GDK online subsystem instance"));
		return false;
	}

	// Get the GDK session interface
	IOnlineSessionPtr SessionInterface = GDKSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::SendInviteToSession(): Failed to get GDK session interface instance"));
		return false;
	}

	// Check if we have a session available with the given name
	FNamedOnlineSession* Session = SessionInterface->GetNamedSession(SessionName);
	if (Session == nullptr)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::SendInviteToSession(): Failed to get a session with name '%s' from the GDK subsystem"), *SessionName.ToString());
		return false;
	}

	// Cast both IDs to AccelByte IDs to extract platform info
	FUniqueNetIdAccelByteUserRef LocalUserAccelByteId = FUniqueNetIdAccelByteUser::CastChecked(LocalUserId);
	FUniqueNetIdAccelByteUserRef InvitedAccelByteId = FUniqueNetIdAccelByteUser::CastChecked(InvitedId);

	// Send an invite to that session from the local user to the invited ID
	return SessionInterface->SendSessionInviteToFriend(LocalUserAccelByteId->GetPlatformUniqueId().ToSharedRef().Get(), SessionName, InvitedAccelByteId->GetPlatformUniqueId().ToSharedRef().Get());
}

void FOnlineAccelByteXboxNativePlatformHandler::OnNativeInviteAccepted(const bool bWasSuccessful, const int32 LocalUserNum, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& SessionResult)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteXboxNativePlatformHandler::OnNativeInviteAccepted(%s, %d, %s, %s)"), LOG_BOOL_FORMAT(bWasSuccessful), LocalUserNum, *UserId->ToString(), *SessionResult.GetSessionIdStr());

	if (!bWasSuccessful)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::OnNativeInviteAccepted(): Failed to extract AccelByte session information from invite"));
		return;
	}

	FString AccelByteSessionType{};
	if (!SessionResult.Session.SessionSettings.Get(FName(AccelByteNativeSessionTypeKey), AccelByteSessionType) || AccelByteSessionType.IsEmpty())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::OnSessionInviteAccepted: Failed to extract AccelByte session information as the AccelByte session type was blank"));
		return;
	}

	FString AccelByteSessionId{};
	if (!SessionResult.Session.SessionSettings.Get(FName(AccelByteNativeSessionIdKey), AccelByteSessionId) || AccelByteSessionId.IsEmpty())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::OnSessionInviteAccepted: Failed to extract AccelByte session information as the AccelByte session ID was blank"));
		return;
	}

	FString AccelBytePartyCode{};
	SessionResult.Session.SessionSettings.Get(FName(AccelByteNativeSessionPartyCodeKey), AccelBytePartyCode);

	// Notify session interface that we have a session to join from native
	// #NOTE Not joining Xbox session here as that will occur after we join the AccelByte session
	OnNativePlatformSessionJoined.Broadcast(UserId->ToString(), AccelByteSessionType, AccelByteSessionId, AccelBytePartyCode);
}

void FOnlineAccelByteXboxNativePlatformHandler::OnFindGDKSessionByIdComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SessionResult, FName SessionName)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelByteXboxNativePlatformHandler::OnFindGDKSessionByIdComplete(%d, %s, %s)"), LocalUserNum, LOG_BOOL_FORMAT(bWasSuccessful), *SessionResult.GetSessionIdStr());

	// Get GDK subsystem instance
	IOnlineSubsystem* GDKSubsystem = IOnlineSubsystem::Get(TEXT("GDK"));
	if (GDKSubsystem == nullptr)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::OnFindGDKSessionByIdComplete(): Failed to get GDK online subsystem instance"));
		return;
	}

	// Get GDK session interface
	IOnlineSessionPtr SessionInterface = GDKSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelByteXboxNativePlatformHandler::OnFindGDKSessionByIdComplete(): Failed to get GDK session interface instance"));
		return;
	}

	// #NOTE For some reason, GDK won't let you join a session without a host, or if not set as dedicated. Let's lie a
	// little bit, copy the session result so that we can modify the dedicated setting, and then join :)
	FOnlineSessionSearchResult FixedSessionResult = SessionResult;
	FixedSessionResult.Session.SessionSettings.bIsDedicated = true;

	// Join session result passed to us, no need to check success state as this is just for sync
	SessionInterface->JoinSession(LocalUserNum, SessionName, FixedSessionResult);
}

#endif // AB_XBOX_NATIVE_PLATFORM_PRESENT
