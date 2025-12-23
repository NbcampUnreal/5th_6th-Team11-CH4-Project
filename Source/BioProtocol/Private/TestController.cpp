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
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] BeginPlay - IsLocalController: true"));

		// EOS 로그인 전에는 VoiceChatUser를 캐싱하지 않음
		// StartProximityVoice()만 시작 (UpdateProximityVoice 내부에서 캐싱 시도)
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

	// 로그인 성공 후 VoiceChatUser 캐싱
	CacheVoiceChatUser();
	if (VoiceChatUser)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice] VoiceChatUser cached after login"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice] Failed to cache VoiceChatUser after login"));
	}
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

	// 내 팀 확인
	ATESTPlayerState* MyPS = GetPlayerState<ATESTPlayerState>();
	const bool bIAmMafia = (MyPS && MyPS->VoiceTeam == EVoiceTeam::Mafia);

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* OtherPawn = *It;
		if (!OtherPawn) continue;
		if (OtherPawn == GetPawn()) continue;

		ATESTPlayerState* OtherPS = Cast<ATESTPlayerState>(OtherPawn->GetPlayerState());
		if (!OtherPS) continue;

		const FString& OtherPuid = OtherPS->EOSPlayerName;
		if (OtherPuid.IsEmpty()) continue;

		// 상대방이 마피아인지 확인
		const bool bOtherIsMafia = (OtherPS->VoiceTeam == EVoiceTeam::Mafia);

		// 마피아끼리는 거리 상관없이 항상 들림
		if (bIAmMafia && bOtherIsMafia)
		{
			VoiceChatUser->SetPlayerVolume(OtherPuid, 1.0f);
			UE_LOG(LogTemp, Verbose, TEXT("[Voice] Mafia-to-Mafia: %s volume = 1.0 (no distance)"), *OtherPuid);
		}
		else
		{
			// 일반 근접 보이스 (시민끼리, 또는 시민-마피아)
			const float Dist = FVector::Dist(ListenerLoc, OtherPawn->GetActorLocation());
			const float Volume = CalcProxVolume01(Dist, ProxMinDist, ProxMaxDist);

			VoiceChatUser->SetPlayerVolume(OtherPuid, Volume);
			UE_LOG(LogTemp, Verbose, TEXT("[Voice] Proximity: %s volume = %.2f (dist: %.1f)"),
				*OtherPuid, Volume, Dist);
		}
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

	// 채널 이름으로 구분해서 저장
	if (ChannelName.Contains(TEXT("Mafia")))
	{
		PrivateChannelName = ChannelName;
		UE_LOG(LogTemp, Warning, TEXT("[Voice] Saved as MAFIA channel: %s"), *ChannelName);
	}
	else if (ChannelName.Contains(TEXT("Citizen")))
	{
		PublicChannelName = ChannelName;
		UE_LOG(LogTemp, Warning, TEXT("[Voice] Saved as PUBLIC channel: %s"), *ChannelName);
	}

	// EOS Trusted Server credentials
	FEOSVoiceChatChannelCredentials Creds;
	Creds.ClientBaseUrl = ClientBaseUrl;
	Creds.ParticipantToken = ParticipantToken;

	const FString JsonCreds = Creds.ToJson();

	UE_LOG(LogTemp, Warning, TEXT("[Voice] Joining channel: %s"), *ChannelName);
	UE_LOG(LogTemp, Warning, TEXT("[Voice] ClientBaseUrl: %s"), *ClientBaseUrl);
	UE_LOG(LogTemp, Warning, TEXT("[Voice] Token length: %d"), ParticipantToken.Len());

	// 채널 참가 (비동기)
	VoiceChatUser->JoinChannel(
		ChannelName,
		JsonCreds,
		EVoiceChatChannelType::NonPositional,
		FOnVoiceChatChannelJoinCompleteDelegate::CreateUObject(
			this, &ATestController::OnVoiceChannelJoined
		)
	);
}


void ATestController::VoiceTransmitToAll()
{
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] VoiceTransmitToAll called"));

	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] VoiceChatUser is NULL"));
		return;
	}

	// 내가 참가한 채널 목록 확인
	TArray<FString> JoinedChannels = VoiceChatUser->GetChannels();
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] Joined channels: %d"), JoinedChannels.Num());
	for (const FString& Ch : JoinedChannels)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT]   - %s"), *Ch);
	}

	VoiceChatUser->TransmitToAllChannels();
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] ✓ Now transmitting to ALL channels"));
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
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] VoiceTransmitToNone called"));

	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] VoiceChatUser is NULL"));
		return;
	}

	VoiceChatUser->TransmitToNoChannels();
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] ✓ Stopped transmitting (muted)"));
}

void ATestController::VoiceTransmitToPublic()
{
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] VoiceTransmitToPublic called"));

	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] VoiceChatUser is NULL"));
		return;
	}

	if (PublicChannelName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] PublicChannelName is EMPTY!"));

		// 디버깅: 참가한 채널 목록 출력
		TArray<FString> JoinedChannels = VoiceChatUser->GetChannels();
		UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] Available channels: %d"), JoinedChannels.Num());
		for (const FString& Ch : JoinedChannels)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT]   - %s"), *Ch);
		}
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] Transmitting to: %s"), *PublicChannelName);

	TSet<FString> Channels = { PublicChannelName };
	VoiceChatUser->TransmitToSpecificChannels(Channels);

	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] ✓ Now transmitting to PUBLIC channel"));
}

void ATestController::VoiceTransmitToMafiaOnly()
{
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] VoiceTransmitToMafiaOnly called"));

	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] VoiceChatUser is NULL"));
		return;
	}

	if (PrivateChannelName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] PrivateChannelName is EMPTY (not mafia?)"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] Transmitting to: %s"), *PrivateChannelName);

	TSet<FString> Channels = { PrivateChannelName };
	VoiceChatUser->TransmitToSpecificChannels(Channels);

	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] ✓ Now transmitting to MAFIA only"));
}

void ATestController::VoiceTransmitToBothChannels()
{
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] VoiceTransmitToBothChannels called"));

	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] VoiceChatUser is NULL"));
		return;
	}

	if (PublicChannelName.IsEmpty() || PrivateChannelName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] Channel names missing (Public: %s, Mafia: %s)"),
			*PublicChannelName, *PrivateChannelName);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] Transmitting to BOTH channels"));
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT]   - %s"), *PublicChannelName);
	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT]   - %s"), *PrivateChannelName);

	TSet<FString> Channels = { PublicChannelName, PrivateChannelName };
	VoiceChatUser->TransmitToSpecificChannels(Channels);

	UE_LOG(LogTemp, Warning, TEXT("[Voice][PTT] ✓ Now transmitting to BOTH channels"));
}


void ATestController::OnVoiceChannelJoined(const FString& ChannelName, const FVoiceChatResult& Result)
{
	if (Result.IsSuccess())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Successfully joined channel: %s"), *ChannelName);

		// ✅ 채널 참가 성공 후 즉시 송신 시작!
		ATESTPlayerState* MyPS = GetPlayerState<ATESTPlayerState>();
		if (MyPS)
		{
			if (MyPS->VoiceTeam == EVoiceTeam::Mafia)
			{
				// 마피아는 기본적으로 전체 채널에만 송신
				VoiceTransmitToPublic();
				UE_LOG(LogTemp, Warning, TEXT("[Voice] Mafia default: transmitting to PUBLIC channel"));
			}
			else
			{
				// 시민은 전체 채널에 송신
				VoiceTransmitToPublic();
				UE_LOG(LogTemp, Warning, TEXT("[Voice] Citizen default: transmitting to PUBLIC channel"));
			}
		}
		else
		{
			// PlayerState가 없으면 모든 채널에 송신
			VoiceTransmitToAll();
			UE_LOG(LogTemp, Warning, TEXT("[Voice] Default: transmitting to ALL channels"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice] ✗ Failed to join channel: %s"), *ChannelName);
		UE_LOG(LogTemp, Error, TEXT("[Voice]   Error: %s"), *Result.ErrorDesc);
		UE_LOG(LogTemp, Error, TEXT("[Voice]   ErrorCode: %s"), *Result.ErrorCode);
	}
}

void ATestController::DebugVoiceStatus()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][Debug] VoiceChatUser is NULL"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] ===== Voice Status ====="));

	// 참가한 채널 목록
	TArray<FString> JoinedChannels = VoiceChatUser->GetChannels();
	UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] Joined Channels: %d"), JoinedChannels.Num());
	for (const FString& Ch : JoinedChannels)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug]   - %s"), *Ch);
	}

	// 송신 중인 채널 목록
	TSet<FString> TransmitChannels = VoiceChatUser->GetTransmitChannels();
	UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] Transmitting to: %d channels"), TransmitChannels.Num());
	for (const FString& Ch : TransmitChannels)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug]   - %s"), *Ch);
	}

	// 마이크 상태
	bool bIsMuted = VoiceChatUser->GetAudioInputDeviceMuted();
	UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] Input Muted: %s"), bIsMuted ? TEXT("YES") : TEXT("NO"));

	// 볼륨
	float InputVolume = VoiceChatUser->GetAudioInputVolume();
	float OutputVolume = VoiceChatUser->GetAudioOutputVolume();
	UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] Input Volume: %.2f"), InputVolume);
	UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] Output Volume: %.2f"), OutputVolume);

	UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] ======================="));
}