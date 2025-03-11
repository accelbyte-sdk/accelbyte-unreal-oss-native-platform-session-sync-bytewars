// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.
#pragma once

#ifdef AB_XBOX_NATIVE_PLATFORM_PRESENT
#include "CoreMinimal.h"
#include "Platform/OnlineAccelByteNativePlatformHandler.h"
#include "OnlineSessionSettings.h"

class FOnlineAccelByteXboxNativePlatformHandler : public IOnlineAccelByteNativePlatformHandler, public TSharedFromThis<FOnlineAccelByteXboxNativePlatformHandler>
{
public:
	FOnlineAccelByteXboxNativePlatformHandler(TSharedRef<FOnlineSubsystemAccelByte, ESPMode::ThreadSafe>&& InSubsystem);

	void Init() override;
	void Deinit() override;
	void Tick(float DeltaTime) override;
	bool JoinSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId) override;
	bool LeaveSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId) override;
	bool SendInviteToSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId, const FUniqueNetId& InvitedId) override;

private:
	void OnNativeInviteAccepted(const bool bWasSuccessful, const int32 LocalUserNum, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& SessionResult);
	void OnFindGDKSessionByIdComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SessionResult, FName SessionName);
};

#endif // AB_XBOX_NATIVE_PLATFORM_PRESENT
