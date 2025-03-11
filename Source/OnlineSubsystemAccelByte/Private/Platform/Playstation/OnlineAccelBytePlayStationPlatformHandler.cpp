// Copyright (c) 2023 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.
#ifdef AB_PLAYSTATION_NATIVE_PLATFORM_PRESENT
#include "OnlineAccelBytePlayStationPlatformHandler.h"
#include "Misc/CoreDelegates.h"
#include "OnlineSubsystemSony.h"
#include "Interfaces/OnlineGameActivityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemAccelByteInternalHelpers.h"
#include "Misc/Base64.h"

FOnlineAccelBytePlayStationNativePlatformHandler::FOnlineAccelBytePlayStationNativePlatformHandler(TSharedRef<FOnlineSubsystemAccelByte, ESPMode::ThreadSafe>&& InSubsystem)
	: IOnlineAccelByteNativePlatformHandler(MoveTempIfPossible(InSubsystem))
{
}

void FOnlineAccelBytePlayStationNativePlatformHandler::Init()
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::Init()"));
	
	FOnlineSubsystemSony* SonySubsystem = FOnlineSubsystemSony::GetSonySubsystem();
	if (SonySubsystem == nullptr)
	{
		// For PlayStation clients specifically, we will handle joining AccelByte sessions using data from the
		// respective OnlineSubsystem since we have enough access to do so.
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::Init(): Could not initialize as we could not get Sony subsystem"));
		return;
	}

	IOnlineGameActivityPtr GameActivityInterface = SonySubsystem->GetGameActivityInterface();
	if (GameActivityInterface.IsValid())
	{
		// For PS5 specifically, sessions can also be joined through activities. Bind here to extract AccelByte session ID from an activity join.
		const FOnGameActivityActivationRequestedDelegate OnGameActivityActivationRequested = FOnGameActivityActivationRequestedDelegate::CreateSP(SharedThis(this), &FOnlineAccelBytePlayStationNativePlatformHandler::OnGameActivityActivationRequested);
		GameActivityInterface->AddOnGameActivityActivationRequestedDelegate_Handle(OnGameActivityActivationRequested);
	}

	IOnlineSessionPtr SessionInterface = SonySubsystem->GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		const FOnSessionUserInviteAcceptedDelegate OnSessionInviteAcceptedDelegate = FOnSessionUserInviteAcceptedDelegate::CreateSP(SharedThis(this), &FOnlineAccelBytePlayStationNativePlatformHandler::OnSessionInviteAccepted);
		SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionInviteAcceptedDelegate);
	}
}

void FOnlineAccelBytePlayStationNativePlatformHandler::Deinit()
{
}

void FOnlineAccelBytePlayStationNativePlatformHandler::Tick(float DeltaTime)
{
}

bool FOnlineAccelBytePlayStationNativePlatformHandler::JoinSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId)
{
	// No need to join a session for PlayStation, as backend handles joins for us. If we did try to join with the PS5 OSS,
	// we would error out.
	return true;
}

bool FOnlineAccelBytePlayStationNativePlatformHandler::LeaveSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId)
{
	// No need to leave a session for PlayStation, as backend handles leaving for us. If we did try to leave with the PS5 OSS,
	// we would error out.
	return true;
}

bool FOnlineAccelBytePlayStationNativePlatformHandler::SendInviteToSession(const FUniqueNetId& LocalUserId, FName SessionName, const FString& SessionId, const FUniqueNetId& InvitedId)
{
	// Backend handles sending invites for PlayStation, just complete
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendInviteToSession(%s, %s, %s)"), *LocalUserId.ToDebugString(), *SessionId, *InvitedId.ToDebugString());
	return true;
}

void FOnlineAccelBytePlayStationNativePlatformHandler::OnSessionInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& SessionInfo)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::OnSessionInviteAccepted(%s, %d, %s, %s)"), LOG_BOOL_FORMAT(bWasSuccessful), ControllerId, *UserId->ToDebugString(), *SessionInfo.GetSessionIdStr())

	if (!bWasSuccessful)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::OnSessionInviteAccepted: Failed to extract AccelByte session information due to delegate reporting failure"));
		return;
	}

	if (!UserId.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::OnSessionInviteAccepted: Failed to extract AccelByte session information as the user ID passed in was invalid"));
		return;
	}

	GetAccelByteSessionDataAndNotify(UserId.ToSharedRef().Get(), SessionInfo.GetSessionIdStr());
}

void FOnlineAccelBytePlayStationNativePlatformHandler::OnGameActivityActivationRequested(const FUniqueNetId& LocalUserId, const FString& ActivityId, const FOnlineSessionSearchResult* SessionInfo)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::OnGameActivityActivationRequested(%s, %s, %s)"), *LocalUserId.ToDebugString(), *ActivityId, (SessionInfo != nullptr) ? *SessionInfo->GetSessionIdStr() : TEXT("NoSession"));

	if (SessionInfo == nullptr)
	{
		// No session data for us to use, abort
		UE_LOG_AB(Verbose, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::OnGameActivityActivationRequested(): No session data from this activity activation, aborting."));
		return;
	}

	GetAccelByteSessionDataAndNotify(LocalUserId, SessionInfo->GetSessionIdStr());
}

void FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(const FUniqueNetId& LocalUserId, const FString& SessionId)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(%s, %s)"), *LocalUserId.ToDebugString(), *SessionId);

	// Send a request to get the customData1 field from the session
	const FString URL = TEXT("/v1/playerSessions?fields=customData1");
	TMap<FString, FString> Headers = {
		{ TEXT("X-PSN-SESSION-MANAGER-SESSION-IDS"), SessionId }
	};

	FString ResponseBody{};
	int32 ErrorCode{};
	bool bSuccess = SendRequest(LocalUserId
		, URL
		, TEXT("GET")
		, Headers
		, ENpApiGroupCrossgen::SessionManager
		, nullptr
		, ResponseBody
		, ErrorCode);

	if (!bSuccess)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Request to get session data from PSN failed"));
		return;
	}

	TSharedRef<TJsonReader<>> PlayerSessionReader = TJsonReaderFactory<>::Create(ResponseBody);
	TSharedPtr<FJsonObject> ResponseJSON{};
	if (!FJsonSerializer::Deserialize(PlayerSessionReader, ResponseJSON))
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to read response as JSON. Response: %s"), *ResponseBody);
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* PlayerSessions{};
	if (!ResponseJSON->TryGetArrayField(TEXT("playerSessions"), PlayerSessions))
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to get player session array from JSON response. Response: %s"), *ResponseBody);
		return;
	}

	if (PlayerSessions->Num() < 1)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to get player session. Response: %s"), *ResponseBody);
		return;
	}

	TSharedPtr<FJsonObject> PlayerSession = (*PlayerSessions)[0]->AsObject();
	if (!PlayerSession.IsValid())
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to get player session JSON object. Response: %s"), *ResponseBody);
		return;
	}

	FString CustomDataBase64{};
	if (!PlayerSession->TryGetStringField(TEXT("customData1"), CustomDataBase64))
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to get custom data JSON field. Response: %s"), *ResponseBody);
		return;
	}

	FString CustomDataJSONStr{};
	if (!FBase64::Decode(CustomDataBase64, CustomDataJSONStr))
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to decode custom data base64 string. Custom data: %s"), *CustomDataBase64);
		return;
	}

	TSharedRef<TJsonReader<>> CustomDataReader = TJsonReaderFactory<>::Create(CustomDataJSONStr);
	TSharedPtr<FJsonObject> CustomDataJSON{};
	if (!FJsonSerializer::Deserialize(CustomDataReader, CustomDataJSON))
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to deserialize custom data JSON object from string. Custom data: %s"), *CustomDataJSONStr);
		return;
	}

	FString AccelByteSessionType{};
	if (!CustomDataJSON->TryGetStringField(AccelByteNativeSessionTypeKey, AccelByteSessionType))
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to extract AccelByte session type from custom data. Custom data: %s"), *CustomDataJSONStr);
		return;
	}

	FString AccelByteSessionId{};
	if (!CustomDataJSON->TryGetStringField(AccelByteNativeSessionIdKey, AccelByteSessionId))
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::GetAccelByteSessionDataAndNotify(): Failed to extract AccelByte session ID from custom data. Custom data: %s"), *CustomDataJSONStr);
		return;
	}

	FString AccelBytePartyCode{};
	CustomDataJSON->TryGetStringField(AccelByteNativeSessionPartyCodeKey, AccelBytePartyCode);

	OnNativePlatformSessionJoined.Broadcast(LocalUserId.ToString(), AccelByteSessionType, AccelByteSessionId, AccelBytePartyCode);
}

bool FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(const FUniqueNetId& LocalUserId
	, const FString& URL
	, const FString& Method
	, TMap<FString, FString> Headers
	, ENpApiGroupCrossgen ApiGroup
	, TSharedPtr<FJsonObject> RequestBody
	, FString& ResponseBody
	, int32& ErrorCode)
{
	UE_LOG_AB(Verbose, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(%s, %s, %s, %s)"), *LocalUserId.ToDebugString(), *URL, *Method, LexToString(ApiGroup));

	// Grab Sony subsystem
	FOnlineSubsystemSony* SonySubsystem = FOnlineSubsystemSony::GetSonySubsystem();
	if (SonySubsystem == nullptr)
	{
		// Need the Sony subsystem instance to get web API handles to send requests
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(): Failed to get Sony subsystem"));
		return false;
	}

	// Grab NP WebAPI pointer to make request
	ISonyWebApi* WebAPI = SonySubsystem->GetWebApi();
	if (WebAPI == nullptr)
	{
		// Need to be able to get a valid WebAPI to make a request
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(): Failed to get WebAPI instance"));
		return false;
	}

	// Get WebAPI user context for local user
	FNpWebApiUserContext WebAPIUserContext = SonySubsystem->GetWebApiUserContext(FUniqueNetIdSony::Cast(LocalUserId));
	if (WebAPIUserContext < 0)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(): Failed to get WebAPI user context for user '%s'"), *LocalUserId.ToDebugString());
		return false;
	}

	// Convert JSON object to string to send through request
	FString RequestBodyStr{};
	if (RequestBody.IsValid())
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&RequestBodyStr);
		if (!FJsonSerializer::Serialize(RequestBody.ToSharedRef(), Writer, true))
		{
			UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(): Failed to convert request body JSON object to string"), *LocalUserId.ToDebugString());
			return false;
		}
	}

	// Finally, convert the request string to UTF8 to send through request
	FTCHARToUTF8 RequestUTF8(*RequestBodyStr);

	int64_t RequestId = INDEX_NONE;
	int32 Result = WebAPI->CreateRequest(RequestId
		, WebAPIUserContext
		, LexToString(ApiGroup)
		, Method
		, URL
		, RequestUTF8.Length()
		, TEXT("")); // Keeping empty as we are just sending JSON, which is the default

	if (Result < 0)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(): Failed to create request"));
		return false;
	}

	for (const TPair<FString, FString>& HeaderKV : Headers)
	{
		WebAPI->AddRequestHeaders(RequestId, HeaderKV.Key, HeaderKV.Value);
	}

	Result = WebAPI->SendRequest(RequestId
		, RequestUTF8.Get()
		, RequestUTF8.Length()
		, ErrorCode);

	if (Result < 0)
	{
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(): Failed to send request to PSN! Error code: %#010x"), Result);
		return false;
	}

	if (ErrorCode < 200 || ErrorCode > 299)
	{
		// Error code was non-success, report and bail
		UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(): Request to PSN failed with error code %d"), ErrorCode);
		return false;
	}

	// Create a buffer to read response data into
	const int32 ResponseBufferBlockSize = 4096;
	TArray<char> ResponseBuffer{};
	ResponseBuffer.Empty(ResponseBufferBlockSize);

	// Slurp all bytes from the response
	int32 BytesRead = 0;
	int32 TotalBytesRead = 0;
	do
	{
		if (ResponseBuffer.Num() - TotalBytesRead < ResponseBufferBlockSize)
		{
			ResponseBuffer.AddUninitialized(ResponseBufferBlockSize);
		}

		BytesRead = WebAPI->ReadData(RequestId, ResponseBuffer.GetData() + TotalBytesRead, ResponseBufferBlockSize);
		if (BytesRead > 0)
		{
			TotalBytesRead += BytesRead;
		}
		else if (BytesRead < 0)
		{
			UE_LOG_AB(Warning, TEXT("FOnlineAccelBytePlayStationNativePlatformHandler::SendRequest(): Failed to read data from request. Error code: %#010x"), BytesRead);
			return false;
		}
	} while (BytesRead > 0);

	// Update length of the response buffer to match the total numbers of bytes that we read
	ResponseBuffer.SetNum(TotalBytesRead, false);

	// Read response buffer as UTF-8 string to get FString
	FUTF8ToTCHAR ResponseConvert(ResponseBuffer.GetData(), ResponseBuffer.Num());
	ResponseBody = FString(ResponseConvert.Length(), ResponseConvert.Get());
	return true;
}

#endif // AB_PLAYSTATION_NATIVE_PLATFORM_PRESENT
