// Fill out your copyright notice in the Description page of Project Settings.
#define STR2(x) #x
#define STR(x) STR2(x)

#pragma message("WITH_VOICECHAT=" STR(WITH_VOICECHAT))
#pragma message("UE_SERVER=" STR(UE_SERVER))

#include "TestController.h"
#include "VoiceChat.h"
#include "../Session/SessionSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "TESTPlayerState.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h" 
#include "GameFramework/PlayerState.h"

#include "IOnlineSubsystemEOS.h"
#include "EOSVoiceChat.h"
#include "EOSVoiceChatTypes.h"




void ATestController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		StartProximityVoice();
	}

}

void ATestController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 종료
	GetWorldTimerManager().ClearTimer(ProximityTimer);
	Super::EndPlay(EndPlayReason);
}

void ATestController::LoginToEOS(int32 Credential)
{
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	IOnlineSubsystem* OSS = Online::GetSubsystem(World);
	if (!OSS)
	{
		return;
	}

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		return;
	}

	const int32 LocalUserNum = 0;

	if (Identity->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn)
	{
		return;
	}

	if (bLoginInProgress)
	{
		return;
	}

	FString DevAuthAddr = TEXT("localhost:8081");
	FParse::Value(FCommandLine::Get(), TEXT("EOSDevAuth="), DevAuthAddr);

	FString DevToken;

	if (Credential < 1) Credential = 1;
	DevToken = FString::Printf(TEXT("DevUser%d"), Credential);

	UE_LOG(LogTemp, Warning, TEXT("[EOS][Login] DevAuth=%s Token=%s"), *DevAuthAddr, *DevToken);

	// 델리게이트 중복 방지
	if (LoginCompleteHandle.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteHandle);
		LoginCompleteHandle.Reset();
	}

	bLoginInProgress = true;

	FOnlineAccountCredentials Creds;
	Creds.Type = TEXT("developer");
	Creds.Id = DevAuthAddr;
	Creds.Token = DevToken;

	LoginCompleteHandle = Identity->AddOnLoginCompleteDelegate_Handle(LocalUserNum, FOnLoginCompleteDelegate::CreateUObject(this, &ATestController::HandleLoginComplete));

	Identity->Login(LocalUserNum, Creds);

}

void ATestController::CreateLobby(const FString& Ip, int32 Port, int32 PublicConnections)
{
	if (!IsLocalController())
	{
		UE_LOG(LogTemp, Error, TEXT("[UI_CreateLobby] Not local controller"));
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	USessionSubsystem* SS = GI->GetSubsystem<USessionSubsystem>();
	if (!SS) return;

	SS->CreateLobbyForDedicated(Ip, Port, PublicConnections);
}

void ATestController::HandleLoginComplete(int32, bool bOk, const FUniqueNetId& Id, const FString& Err)
{
	UE_LOG(LogTemp, Warning, TEXT("[EOS][Login] ok=%d user=%s err=%s"), bOk, *Id.ToString(), *Err);

	if (UWorld* World = GetWorld())
	{
		if (IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World))
		{
			if (LoginCompleteHandle.IsValid())
			{
				Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
				LoginCompleteHandle.Reset();
			}
		}
	}

	bLoginInProgress = false;

	if (!bOk)
		return;

	Server_SetEOSPlayerName(Id.ToString());
}

void ATestController::Server_StartGameSession_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	USessionSubsystem* SessionSub = GI->GetSubsystem<USessionSubsystem>();
	if (!SessionSub)
	{
		return;
	}

	SessionSub->StartGameSession(); 
}

void ATestController::Server_SetReady_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	ATESTPlayerState* PS = GetPlayerState<ATESTPlayerState>();
	if (!PS)
	{
		return;
	}

	PS->bisReady = !PS->bisReady;

}

void ATestController::StartProximityVoice()
{
	GetWorldTimerManager().SetTimer(
		ProximityTimer,
		this,
		&ATestController::UpdateProximityVoice,
		ProxUpdateInterval,
		true
	);
}

void ATestController::UpdateProximityVoice()
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	FVector ListenerLoc;
	if (APawn* MyPawn = GetPawn())
	{
		ListenerLoc = MyPawn->GetActorLocation();
	}
	else if (PlayerCameraManager)
	{
		ListenerLoc = PlayerCameraManager->GetCameraLocation();
	}
	else
	{
		return;
	}

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* OtherPawn = *It;
		if (!OtherPawn) continue;
		if (OtherPawn == GetPawn()) continue;

		ATESTPlayerState* OtherPS = Cast<ATESTPlayerState>(OtherPawn->GetPlayerState());
		if (!OtherPS) continue;

		const FString& OtherPuid = OtherPS->EOSPlayerName; // 00023... 형태
		if (OtherPuid.IsEmpty()) continue;

		const float Dist = FVector::Dist(ListenerLoc, OtherPawn->GetActorLocation());
		const float Volume = CalcProxVolume01(Dist, ProxMinDist, ProxMaxDist);

		VoiceChatUser->SetPlayerVolume(OtherPuid, Volume);
	}

}

float ATestController::CalcProxVolume01(float Dist, float MinD, float MaxD)
{
	if (Dist <= MinD) return 1.0f;
	if (Dist >= MaxD) return 0.0f;

	const float Alpha = (Dist - MinD) / (MaxD - MinD); // 0..1
	return FMath::Clamp(1.0f - (Alpha * Alpha), 0.0f, 1.0f);
}

void ATestController::Server_SetEOSPlayerName_Implementation(const FString& InEOSPlayerName)
{
	if (!HasAuthority())
		return;

	ATESTPlayerState* PS = GetPlayerState<ATESTPlayerState>();
	if (!PS)
		return;

	PS->Server_SetEOSPlayerName(InEOSPlayerName);
}

void ATestController::CacheVoiceChatUser()
{
	if (VoiceChatUser)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	// EOS 서브시스템을 명시적으로 가져오는 게 안전함
	IOnlineSubsystem* OSS = Online::GetSubsystem(World, TEXT("EOS"));
	if (!OSS)
		return;

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid())
		return;

	TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
	if (!LocalUserId.IsValid())
		return;

	// 문서에 나온 EOS 전용 인터페이스
	IOnlineSubsystemEOS* EOS = static_cast<IOnlineSubsystemEOS*>(OSS);
	if (!EOS)
		return;

	VoiceChatUser = EOS->GetVoiceChatUserInterface(*LocalUserId);
	if (VoiceChatUser)
	{
		UE_LOG(LogTemp, Log, TEXT("[Voice] Cached IVoiceChatUser for LocalUserId=%s"), *LocalUserId->ToString());
	}

}

void ATestController::Client_JoinPrivateVoiceChannel_Implementation(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	JoinPrivateVoiceChannel_Local(ChannelName, ClientBaseUrl, ParticipantToken);
}

void ATestController::JoinPrivateVoiceChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice] JoinPrivate failed: VoiceChatUser null"));
		return;
	}

	PrivateChannelName = ChannelName;

	// EOS Trusted Server credentials
	FEOSVoiceChatChannelCredentials Creds;
	Creds.ClientBaseUrl = ClientBaseUrl;
	Creds.ParticipantToken = ParticipantToken;

	const FString JsonCreds = Creds.ToJson();

	UE_LOG(LogTemp, Log, TEXT("[Voice] Joining private channel: %s"), *ChannelName);
	UE_LOG(LogTemp, Log, TEXT("[Voice] ClientBaseUrl: %s"), *ClientBaseUrl);

	// 채널 참가 (비동기)
	VoiceChatUser->JoinChannel(
		ChannelName,
		JsonCreds,
		EVoiceChatChannelType::NonPositional,
		FOnVoiceChatChannelJoinCompleteDelegate::CreateLambda(
			[ChannelName](const FString& JoinedChannel, const FVoiceChatResult& Result)
			{
				if (Result.IsSuccess())
				{
					UE_LOG(LogTemp, Log, TEXT("[Voice] Successfully joined private channel: %s"),
						*JoinedChannel);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[Voice] Failed to join channel: %s, Error: %s"),
						*JoinedChannel, *Result.ErrorDesc);
				}
			}
		)
	);
}


void ATestController::VoiceTransmitToAll()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser) return;

	VoiceChatUser->TransmitToAllChannels(); // docs :contentReference[oaicite:2]{index=2}
}

void ATestController::VoiceTransmitToChannel(const FString& ChannelName)
{
	CacheVoiceChatUser();
	if (!VoiceChatUser) return;

	TSet<FString> Channels = { ChannelName };
	VoiceChatUser->TransmitToSpecificChannels(Channels);
}

void ATestController::VoiceTransmitToNone()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser) return;

	VoiceChatUser->TransmitToNoChannels(); // docs :contentReference[oaicite:4]{index=4}
}