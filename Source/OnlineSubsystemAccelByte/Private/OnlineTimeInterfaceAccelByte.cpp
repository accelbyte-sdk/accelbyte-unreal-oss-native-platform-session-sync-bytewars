﻿// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "OnlineTimeInterfaceAccelByte.h"
#include "OnlineSubsystemUtils.h"
#include "Core/AccelByteRegistry.h"
#include "AsyncTasks/Time/OnlineAsyncTaskAccelByteGetServerTime.h"

FOnlineTimeAccelByte::FOnlineTimeAccelByte(FOnlineSubsystemAccelByte* InSubsystem) 
	: AccelByteSubsystem(InSubsystem)
{
}

bool FOnlineTimeAccelByte::GetFromSubsystem(const IOnlineSubsystem* Subsystem, FOnlineTimeAccelBytePtr& OutInterfaceInstance)
{
	OutInterfaceInstance = StaticCastSharedPtr<FOnlineTimeAccelByte>(Subsystem->GetTimeInterface());
	return OutInterfaceInstance.IsValid();
}

bool FOnlineTimeAccelByte::GetFromWorld(const UWorld* World, FOnlineTimeAccelBytePtr& OutInterfaceInstance)
{
	const IOnlineSubsystem* Subsystem = ::Online::GetSubsystem(World);
	if (Subsystem == nullptr)
	{
		OutInterfaceInstance = nullptr;
		return false;
	}

	return GetFromSubsystem(Subsystem, OutInterfaceInstance);
}

bool FOnlineTimeAccelByte::QueryServerUtcTime()
{
	AccelByteSubsystem->CreateAndDispatchAsyncTaskParallel<FOnlineAsyncTaskAccelByteGetServerTime>(AccelByteSubsystem);
	return true;
}

FString FOnlineTimeAccelByte::GetLastServerUtcTime()
{
	return AccelByte::FRegistry::TimeManager.GetCachedServerTime().ToString();
}

FString FOnlineTimeAccelByte::GetCurrentServerUtcTime()
{
	return AccelByte::FRegistry::TimeManager.GetCurrentServerTime().ToString();
}
