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

	UE_LOG(LogTemp, Warning, TEXT("[Controller] BeginPlay - Role: %d, IsLocal: %d, Map: %s"),
		(int32)GetLocalRole(),
		IsLocalController() ? 1 : 0,
		*GetWorld()->GetMapName());

	if (IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Debug] BeginPlay - IsLocalController: true"));
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
		return;

	UGameInstance* GI = GetGameInstance();
	if (!GI)
		return;

	USessionSubsystem* SessionSub = GI->GetSubsystem<USessionSubsystem>();
	if (!SessionSub)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[Controller] Requesting travel to game map"));
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

	// ✅ Listener (내 위치) 정보
	FVector ListenerLoc;
	FVector ListenerForward = FVector::ForwardVector;
	FVector ListenerUp = FVector::UpVector;

	if (APawn* MyPawn = GetPawn())
	{
		ListenerLoc = MyPawn->GetActorLocation();
		ListenerForward = MyPawn->GetActorForwardVector();
		ListenerUp = MyPawn->GetActorUpVector();
	}
	else if (PlayerCameraManager)
	{
		ListenerLoc = PlayerCameraManager->GetCameraLocation();
		ListenerForward = PlayerCameraManager->GetActorForwardVector();
		ListenerUp = PlayerCameraManager->GetActorUpVector();
	}
	else
	{
		return;
	}

	// ✅ 내 팀 확인
	ATESTPlayerState* MyPS = GetPlayerState<ATESTPlayerState>();
	const bool bIAmMafia = (MyPS && MyPS->VoiceTeam == EVoiceTeam::Mafia);

	// ✅ PUBLIC 채널 (Positional) - 3D 위치 + 거리별 볼륨
	if (!PublicGameChannelName.IsEmpty())
	{
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* OtherPawn = *It;
			if (!OtherPawn || OtherPawn == GetPawn())
				continue;

			ATESTPlayerState* OtherPS = Cast<ATESTPlayerState>(OtherPawn->GetPlayerState());
			if (!OtherPS || OtherPS->EOSPlayerName.IsEmpty())
				continue;

			const bool bOtherIsMafia = (OtherPS->VoiceTeam == EVoiceTeam::Mafia);

			// ✅ 거리 계산
			FVector SpeakerLoc = OtherPawn->GetActorLocation();
			float Distance = FVector::Dist(ListenerLoc, SpeakerLoc);

			// ✅ 거리별 볼륨 계산
			float Volume = CalcProxVolume01(Distance, ProxMinDist, ProxMaxDist);

			// ✅ 3D 위치 업데이트 (공간 오디오)
			VoiceChatUser->Set3DPosition(
				PublicGameChannelName,
				SpeakerLoc,
				ListenerLoc,
				ListenerForward,
				ListenerUp
			);

			// ✅ CRITICAL: 마피아끼리는 항상 최대 볼륨 유지
			if (bIAmMafia && bOtherIsMafia)
			{
				Volume = 1.0f;  // 마피아끼리는 거리 무관
				UE_LOG(LogTemp, VeryVerbose,
					TEXT("[Voice][Proximity] MAFIA override: Player=%s Vol=1.0 (ignore distance)"),
					*OtherPS->EOSPlayerName);
			}

			// ✅ 볼륨 설정
			VoiceChatUser->SetPlayerVolume(OtherPS->EOSPlayerName, Volume);

			UE_LOG(LogTemp, Log,
				TEXT("[Voice][Proximity] Player=%s IsMafia=%d Dist=%.0fcm Vol=%.2f"),
				*OtherPS->EOSPlayerName, bOtherIsMafia ? 1 : 0, Distance, Volume);
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


// 송신 제어

void ATestController::VoiceTransmitToNone()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	VoiceChatUser->TransmitToNoChannels();
	UE_LOG(LogTemp, Log, TEXT("[Voice][PTT] ✓ Stopped transmitting"));
}

void ATestController::VoiceTransmitToPublic()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	FString ChannelToUse = PublicGameChannelName.IsEmpty() ? LobbyChannelName : PublicGameChannelName;

	if (ChannelToUse.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] No public channel available"));
		return;
	}

	TSet<FString> Channels = { ChannelToUse };
	VoiceChatUser->TransmitToSpecificChannels(Channels);

	UE_LOG(LogTemp, Log, TEXT("[Voice][PTT] ✓ Transmitting to PUBLIC: %s"), *ChannelToUse);
}

void ATestController::VoiceTransmitToMafiaOnly()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	if (MafiaGameChannelName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] No mafia channel (not mafia or not in game?)"));
		return;
	}

	TSet<FString> Channels = { MafiaGameChannelName };
	VoiceChatUser->TransmitToSpecificChannels(Channels);

	UE_LOG(LogTemp, Log, TEXT("[Voice][PTT] ✓ Transmitting to MAFIA ONLY"));
}

void ATestController::VoiceTransmitToBothChannels()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	if (PublicGameChannelName.IsEmpty() || MafiaGameChannelName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][PTT] Missing channels for both transmission"));
		return;
	}

	TSet<FString> Channels = { PublicGameChannelName, MafiaGameChannelName };
	VoiceChatUser->TransmitToSpecificChannels(Channels);

	UE_LOG(LogTemp, Log, TEXT("[Voice][PTT] ✓ Transmitting to BOTH channels"));
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

// 로비 채널 관련

void ATestController::Client_JoinLobbyChannel_Implementation(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	JoinLobbyChannel_Local(ChannelName, ClientBaseUrl, ParticipantToken);
}

void ATestController::JoinLobbyChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Lobby] JoinLobbyChannel failed: VoiceChatUser null"));
		return;
	}

	LobbyChannelName = ChannelName;
	UE_LOG(LogTemp, Warning, TEXT("[Voice][Lobby] ✓ Joining LOBBY channel: %s"), *ChannelName);

	// EOS credentials (로비는 SessionSubsystem에서 자동 생성)
	FEOSVoiceChatChannelCredentials Creds;
	Creds.ClientBaseUrl = ClientBaseUrl;
	Creds.ParticipantToken = ParticipantToken;

	const FString JsonCreds = Creds.ToJson();

	VoiceChatUser->JoinChannel(
		ChannelName,
		JsonCreds,
		EVoiceChatChannelType::NonPositional,
		FOnVoiceChatChannelJoinCompleteDelegate::CreateLambda(
			[this, ChannelName](const FString& JoinedChannel, const FVoiceChatResult& Result)
			{
				if (Result.IsSuccess())
				{
					UE_LOG(LogTemp, Warning, TEXT("[Voice][Lobby] ✓ Successfully joined LOBBY channel"));
					// 로비에서는 자동으로 송신 시작
					VoiceChatUser->TransmitToAllChannels();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[Voice][Lobby] ✗ Failed to join: %s"), *Result.ErrorDesc);
				}
			}
		)
	);
}

void ATestController::LeaveLobbyChannel()
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser || LobbyChannelName.IsEmpty())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[Voice][Lobby] Leaving lobby channel: %s"), *LobbyChannelName);

	VoiceChatUser->LeaveChannel(
		LobbyChannelName,
		FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
			[this](const FString& LeftChannel, const FVoiceChatResult& Result)
			{
				if (Result.IsSuccess())
				{
					UE_LOG(LogTemp, Warning, TEXT("[Voice][Lobby] ✓ Left lobby channel"));
					LobbyChannelName.Empty();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[Voice][Lobby] ✗ Failed to leave: %s"), *Result.ErrorDesc);
				}
			}
		)
	);
}

// 게임 채널 관련

void ATestController::Client_JoinGameChannel_Implementation(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	JoinGameChannel_Local(ChannelName, ClientBaseUrl, ParticipantToken);
}

void ATestController::JoinGameChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Game] JoinGameChannel failed: VoiceChatUser null"));
		return;
	}

	bool bIsMafiaChannel = ChannelName.Contains(TEXT("Mafia"));
	bool bIsPublicChannel = ChannelName.Contains(TEXT("Citizen")) || ChannelName.Contains(TEXT("Public"));

	// ✅ 채널 타입 결정
	EVoiceChatChannelType ChannelType = EVoiceChatChannelType::NonPositional;

	if (bIsMafiaChannel)
	{
		MafiaGameChannelName = ChannelName;
		ChannelType = EVoiceChatChannelType::NonPositional;  // 마피아는 일반 채널
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Game] ✓ Joining MAFIA channel (NonPositional): %s"), *ChannelName);
	}
	else if (bIsPublicChannel)
	{
		PublicGameChannelName = ChannelName;
		ChannelType = EVoiceChatChannelType::Positional;  // ✅ 시민은 3D 채널
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Game] ✓ Joining PUBLIC channel (Positional 3D): %s"), *ChannelName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Game] ⚠ Unknown channel type: %s"), *ChannelName);
		return;
	}

	FEOSVoiceChatChannelCredentials Creds;
	Creds.ClientBaseUrl = ClientBaseUrl;
	Creds.ParticipantToken = ParticipantToken;

	const FString JsonCreds = Creds.ToJson();

	VoiceChatUser->JoinChannel(
		ChannelName,
		JsonCreds,
		ChannelType,  // ✅ 채널 타입 적용
		FOnVoiceChatChannelJoinCompleteDelegate::CreateUObject(
			this, &ATestController::OnVoiceChannelJoined
		)
	);
}

void ATestController::LeaveGameChannels()
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[Voice][Game] Leaving game channels..."));

	// 공개 채널 나가기
	if (!PublicGameChannelName.IsEmpty())
	{
		VoiceChatUser->LeaveChannel(
			PublicGameChannelName,
			FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
				[this](const FString& LeftChannel, const FVoiceChatResult& Result)
				{
					if (Result.IsSuccess())
					{
						UE_LOG(LogTemp, Log, TEXT("[Voice][Game] ✓ Left PUBLIC channel"));
						PublicGameChannelName.Empty();
					}
				}
			)
		);
	}

	// 마피아 채널 나가기
	if (!MafiaGameChannelName.IsEmpty())
	{
		VoiceChatUser->LeaveChannel(
			MafiaGameChannelName,
			FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
				[this](const FString& LeftChannel, const FVoiceChatResult& Result)
				{
					if (Result.IsSuccess())
					{
						UE_LOG(LogTemp, Log, TEXT("[Voice][Game] ✓ Left MAFIA channel"));
						MafiaGameChannelName.Empty();
					}
				}
			)
		);
	}
}

void ATestController::OnVoiceChannelJoined(const FString& ChannelName, const FVoiceChatResult& Result)
{
	if (Result.IsSuccess())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice][Game] ✓ Successfully joined: %s"), *ChannelName);

		// ✅ 게임 시작 시 기본 송신 채널 설정
		ATESTPlayerState* MyPS = GetPlayerState<ATESTPlayerState>();
		if (MyPS && MyPS->VoiceTeam == EVoiceTeam::Mafia)
		{
			// 마피아는 공개 채널로 기본 송신
			VoiceTransmitToPublic();
			UE_LOG(LogTemp, Log, TEXT("[Voice][Game] Mafia default: PUBLIC channel"));
		}
		else
		{
			// 시민은 공개 채널로만 송신
			VoiceTransmitToPublic();
			UE_LOG(LogTemp, Log, TEXT("[Voice][Game] Citizen: PUBLIC channel"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice][Game] ✗ Failed to join: %s"), *Result.ErrorDesc);
	}
}