// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BioPlayerController.h"
#include "UI/BioPlayerHUD.h"
#include "Blueprint/UserWidget.h"
#include "VoiceChat.h"
#include "../Session/SessionSubsystem.h"
#include "Game/BioPlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "IOnlineSubsystemEOS.h"
#include "EOSVoiceChat.h"
#include "EOSVoiceChatTypes.h"
#include "Net/VoiceConfig.h"


ABioPlayerController::ABioPlayerController()
{
	bReplicates = true;
	UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerController Constructor - bReplicates: %s"),
		bReplicates ? TEXT("TRUE") : TEXT("FALSE"));
}

//void ABioPlayerController::BeginPlay()
//{
//	Super::BeginPlay();
//
//	UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerController BeginPlay - Authority: %s, LocalController: %s"),
//		HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
//		IsLocalController() ? TEXT("YES") : TEXT("NO"));
//
//	// UI 초기화 (로컬 컨트롤러만)
//	if (IsLocalController())
//	{
//		if (BioHUDClass)
//		{
//			InGameHUD = CreateWidget<UBioPlayerHUD>(this, BioHUDClass);
//
//			if (InGameHUD)
//			{
//				InGameHUD->AddToViewport();
//			}
//		}
//		else
//		{
//			UE_LOG(LogTemp, Error, TEXT("BioHUDClass is NOT set in BioPlayerController!"));
//		}
//
//		// 근접 보이스 시작
//		StartProximityVoice();
//	}
//
//	// PlayerState 확인
//	ABioPlayerState* PS = GetPlayerState<ABioPlayerState>();
//	if (PS)
//	{
//		UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerState Found: %s, Role: %d"),
//			*PS->GetName(), (int32)PS->GameRole);
//	}
//	else
//	{
//		UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerState NOT FOUND - Will retry"));
//
//		FTimerHandle RetryTimer;
//		GetWorldTimerManager().SetTimer(RetryTimer, [this]()
//			{
//				ABioPlayerState* RetryPS = GetPlayerState<ABioPlayerState>();
//				if (RetryPS)
//				{
//					UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerState Found (Retry): %s"), *RetryPS->GetName());
//				}
//				else
//				{
//					UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerState STILL NOT FOUND!"));
//				}
//			}, 1.0f, false);
//	}
//
//	// 게임 맵에서는 송신 중지 상태로 시작
//	UWorld* World = GetWorld();
//	if (World)
//	{
//		FString MapName = World->GetMapName();
//		if (MapName.StartsWith(TEXT("UEDPIE_")))
//		{
//			int32 UnderscoreIndex;
//			if (MapName.FindChar('_', UnderscoreIndex))
//			{
//				MapName = MapName.RightChop(UnderscoreIndex + 1);
//			}
//		}
//
//		if (MapName.Contains(TEXT("/")))
//		{
//			int32 LastSlashIndex;
//			MapName.FindLastChar('/', LastSlashIndex);
//			MapName = MapName.RightChop(LastSlashIndex + 1);
//		}
//
//		if (MapName.Contains(TEXT("TestSession")) || MapName.Contains(TEXT("GameMap")))
//		{
//			VoiceTransmitToNone();
//		}
//	}
//}


void ABioPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (World)
	{
		ENetMode NetMode = World->GetNetMode();
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		UE_LOG(LogTemp, Error, TEXT("BioPlayerController::BeginPlay"));
		UE_LOG(LogTemp, Error, TEXT("NetMode: %d"), (int32)NetMode);
		UE_LOG(LogTemp, Error, TEXT("========================================"));
	}

	if (IsLocalController())
	{
		// UI 초기화
		if (BioHUDClass)
		{
			InGameHUD = CreateWidget<UBioPlayerHUD>(this, BioHUDClass);
			if (InGameHUD)
			{
				InGameHUD->AddToViewport();
			}
		}

		// 클라이언트만 VoIP 시작
		if (World && World->GetNetMode() == NM_Client)
		{
			// 2초 후 VOIPTalker 생성
			FTimerHandle InitTimer;
			GetWorldTimerManager().SetTimer(InitTimer, [this]()
				{
					if (PlayerState)
					{
						CreateVOIPTalker();

						// 1초 후 음성 시작
						FTimerHandle VoiceTimer;
						GetWorldTimerManager().SetTimer(VoiceTimer, [this]()
							{
								StartNativeVoIP();
							}, 1.0f, false);
					}
				}, 2.0f, false);
		}
	}
}

void ABioPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ProximityTimer);
	Super::EndPlay(EndPlayReason);
}

void ABioPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(LogTemp, Error, TEXT("=== OnPossess called ==="));
	UE_LOG(LogTemp, Error, TEXT("Pawn: %s"), InPawn ? *InPawn->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("VOIPTalkerComponent: %s"), VOIPTalkerComponent ? TEXT("EXISTS") : TEXT("NULL"));

	if (IsLocalController() && VOIPTalkerComponent && InPawn)
	{
		VOIPTalkerComponent->Settings.ComponentToAttachTo = InPawn->GetRootComponent();
		UE_LOG(LogTemp, Warning, TEXT("[VoIP] VOIPTalker attached to new Pawn in OnPossess"));
	}
}

// ========================================
// EOS Login
// ========================================

void ABioPlayerController::LoginToEOS(int32 Credential)
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
	if (Identity->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn || bLoginInProgress)
	{
		return;
	}

	FString DevAuthAddr = TEXT("localhost:8081");
	FParse::Value(FCommandLine::Get(), TEXT("EOSDevAuth="), DevAuthAddr);

	FString DevToken = FString::Printf(TEXT("DevUser%d"), FMath::Max(1, Credential));

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

	LoginCompleteHandle = Identity->AddOnLoginCompleteDelegate_Handle(LocalUserNum,
		FOnLoginCompleteDelegate::CreateUObject(this, &ABioPlayerController::HandleLoginComplete));

	Identity->Login(LocalUserNum, Creds);
}

void ABioPlayerController::HandleLoginComplete(int32, bool bOk, const FUniqueNetId& Id, const FString& Err)
{
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
	{
		UE_LOG(LogTemp, Error, TEXT("[EOS] ✗ Login failed: %s"), *Err);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[EOS] ✓ Login successful: %s"), *Id.ToString());

	Server_SetEOSPlayerName(Id.ToString());
	CacheVoiceChatUser();
}

// ========================================
// Session Management
// ========================================


void ABioPlayerController::CreateLobby(const FString& Ip, int32 Port, int32 PublicConnections)
{
	if (!IsLocalController())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	USessionSubsystem* SS = GI->GetSubsystem<USessionSubsystem>();
	if (!SS) return;

	SS->CreateLobbyForDedicated(Ip, Port, PublicConnections);
}

void ABioPlayerController::Server_StartGameSession_Implementation()
{
	UE_LOG(LogTemp, Error, TEXT("🔴 SERVER RPC: Server_StartGameSession_Implementation CALLED"));

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("🔴 ERROR: Server_StartGameSession has NO Authority!"));
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("🔴 Server has Authority, getting GameInstance..."));

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("🔴 ERROR: GameInstance is NULL!"));
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("🔴 GameInstance found, getting SessionSubsystem..."));

	USessionSubsystem* SessionSub = GI->GetSubsystem<USessionSubsystem>();
	if (SessionSub)
	{
		UE_LOG(LogTemp, Error, TEXT("🔴 SessionSubsystem found, calling StartGameSession"));
		SessionSub->StartGameSession();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🔴 ERROR: SessionSubsystem is NULL!"));
	}
}

void ABioPlayerController::Server_SetReady_Implementation()
{
	UE_LOG(LogTemp, Error, TEXT("🟢 SERVER RPC: Server_SetReady_Implementation CALLED"));

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("🟢 ERROR: Server_SetReady has NO Authority!"));
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("🟢 Server has Authority, getting PlayerState..."));

	ABioPlayerState* PS = GetPlayerState<ABioPlayerState>();
	if (PS)
	{
		UE_LOG(LogTemp, Error, TEXT("🟢 PlayerState found: %s"), *PS->GetName());
		UE_LOG(LogTemp, Error, TEXT("🟢 Current bIsReady: %s"), PS->bIsReady ? TEXT("TRUE") : TEXT("FALSE"));

		PS->bIsReady = !PS->bIsReady;

		UE_LOG(LogTemp, Error, TEXT("🟢 New bIsReady: %s"), PS->bIsReady ? TEXT("TRUE") : TEXT("FALSE"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🟢 ERROR: PlayerState is NULL!"));
	}
}

void ABioPlayerController::Server_SetEOSPlayerName_Implementation(const FString& InEOSPlayerName)
{
	UE_LOG(LogTemp, Error, TEXT("🟡 SERVER RPC: Server_SetEOSPlayerName_Implementation CALLED - Name: %s"), *InEOSPlayerName);

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("🟡 ERROR: Server_SetEOSPlayerName has NO Authority!"));
		return;
	}

	ABioPlayerState* PS = GetPlayerState<ABioPlayerState>();
	if (PS)
	{
		UE_LOG(LogTemp, Error, TEXT("🟡 PlayerState found, setting EOS name"));
		PS->Server_SetEOSPlayerName(InEOSPlayerName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🟡 ERROR: PlayerState is NULL!"));
	}
}


// ========================================
// Native VoIP
// ========================================

void ABioPlayerController::StartNativeVoIP()
{
	if (!IsLocalController())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[VoIP] Starting Native VoIP..."));

	// StartTalking 호출
	StartTalking();

	// 콘솔 명령어로도 시도
	ConsoleCommand(TEXT("ToggleSpeaking 1"));

	// 0.5초 후 상태 확인
	FTimerHandle CheckTimer;
	GetWorldTimerManager().SetTimer(CheckTimer, [this]()
		{
			if (VOIPTalkerComponent)
			{
				if (VOIPTalkerComponent->IsActive())
				{
					UE_LOG(LogTemp, Warning, TEXT("[VoIP] ✅✅✅ VoIP WORKING!"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[VoIP] ❌ Still not active - Check INI settings!"));
					UE_LOG(LogTemp, Error, TEXT("[VoIP] Make sure [Voice] bEnabled=true in DefaultEngine.ini"));
				}
			}
		}, 0.5f, false);
}

void ABioPlayerController::StopNativeVoIP()
{
	if (!IsLocalController())
		return;

	StopTalking();

	UE_LOG(LogTemp, Warning, TEXT("[Native VoIP] Stopped talking"));
}

void ABioPlayerController::CreateVOIPTalker()
{
	UE_LOG(LogTemp, Error, TEXT("=== CreateVOIPTalker START ==="));

	if (!IsLocalController())
	{
		UE_LOG(LogTemp, Error, TEXT("[VoIP] ❌ Not local controller"));
		return;
	}

	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("[VoIP] ❌ PlayerState is NULL"));
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("[VoIP] PlayerState exists: %s"), *PlayerState->GetName());

	// 이미 생성되어 있는지 확인
	VOIPTalkerComponent = PlayerState->FindComponentByClass<UVOIPTalker>();

	if (VOIPTalkerComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VoIP] ✅ VOIPTalker already exists in PlayerState"));
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("[VoIP] VOIPTalker not found, creating new one..."));

	// VOIPTalker 생성
	VOIPTalkerComponent = NewObject<UVOIPTalker>(
		PlayerState,
		UVOIPTalker::StaticClass(),
		TEXT("VOIPTalker")
	);

	if (!VOIPTalkerComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[VoIP] ❌❌❌ Failed to create VOIPTalker with NewObject!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[VoIP] ✅ VOIPTalker created with NewObject"));

	// 3D 위치 기반 음성 설정
	FVoiceSettings Settings;

	// Character의 Root Component에 붙임
	APawn* MyPawn = GetPawn();
	if (MyPawn)
	{
		Settings.ComponentToAttachTo = MyPawn->GetRootComponent();
		UE_LOG(LogTemp, Warning, TEXT("[VoIP] VOIPTalker attached to Pawn root: %s"),
			*MyPawn->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VoIP] ⚠️ No Pawn yet"));
	}

	Settings.AttenuationSettings = nullptr;
	VOIPTalkerComponent->Settings = Settings;

	UE_LOG(LogTemp, Error, TEXT("[VoIP] Calling RegisterComponent()..."));
	VOIPTalkerComponent->RegisterComponent();

	UE_LOG(LogTemp, Error, TEXT("[VoIP] Calling RegisterWithPlayerState()..."));
	VOIPTalkerComponent->RegisterWithPlayerState(PlayerState);

	UE_LOG(LogTemp, Warning, TEXT("[VoIP] ✅✅✅ VOIPTalker fully registered"));

	// 확인
	UVOIPTalker* FoundTalker = PlayerState->FindComponentByClass<UVOIPTalker>();
	if (FoundTalker)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VoIP] ✅ Verification: VOIPTalker found in PlayerState!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[VoIP] ❌ Verification FAILED: VOIPTalker NOT found in PlayerState!"));
	}
}



// ========================================
// Voice Chat Core
// ========================================

void ABioPlayerController::CacheVoiceChatUser()
{
	if (VoiceChatUser)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	IOnlineSubsystem* OSS = Online::GetSubsystem(World, TEXT("EOS"));
	if (!OSS)
		return;

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid())
		return;

	TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
	if (!LocalUserId.IsValid())
		return;

	IOnlineSubsystemEOS* EOS = static_cast<IOnlineSubsystemEOS*>(OSS);
	if (EOS)
	{
		VoiceChatUser = EOS->GetVoiceChatUserInterface(*LocalUserId);
	}
}

// ========================================
// Voice Channel Management
// ========================================

void ABioPlayerController::Client_JoinLobbyChannel_Implementation(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	JoinLobbyChannel_Local(ChannelName, ClientBaseUrl, ParticipantToken);
}

void ABioPlayerController::JoinLobbyChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	LobbyChannelName = ChannelName;

	FEOSVoiceChatChannelCredentials Creds;
	Creds.ClientBaseUrl = ClientBaseUrl;
	Creds.ParticipantToken = ParticipantToken;

	VoiceChatUser->JoinChannel(ChannelName, Creds.ToJson(), EVoiceChatChannelType::NonPositional,
		FOnVoiceChatChannelJoinCompleteDelegate::CreateLambda(
			[this](const FString& JoinedChannel, const FVoiceChatResult& Result)
			{
				if (Result.IsSuccess())
				{
					UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Joined lobby channel"));
					VoiceChatUser->TransmitToAllChannels();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[Voice] ✗ Failed to join lobby: %s"), *Result.ErrorDesc);
				}
			}
		)
	);
}

void ABioPlayerController::LeaveLobbyChannel()
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser || LobbyChannelName.IsEmpty())
		return;

	VoiceChatUser->LeaveChannel(LobbyChannelName,
		FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
			[this](const FString& LeftChannel, const FVoiceChatResult& Result)
			{
				if (Result.IsSuccess())
				{
					UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Left lobby channel"));
					LobbyChannelName.Empty();
				}
			}
		)
	);
}

void ABioPlayerController::Client_JoinGameChannel_Implementation(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	JoinGameChannel_Local(ChannelName, ClientBaseUrl, ParticipantToken);
}

void ABioPlayerController::JoinGameChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	MafiaGameChannelName = ChannelName;

	FEOSVoiceChatChannelCredentials Creds;
	Creds.ClientBaseUrl = ClientBaseUrl;
	Creds.ParticipantToken = ParticipantToken;

	// 마피아 채널은 NonPositional (거리 무관)
	VoiceChatUser->JoinChannel(ChannelName, Creds.ToJson(), EVoiceChatChannelType::NonPositional,
		FOnVoiceChatChannelJoinCompleteDelegate::CreateLambda(
			[this, ChannelName](const FString& JoinedChannel, const FVoiceChatResult& Result)
			{
				if (Result.IsSuccess())
				{
					UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Joined mafia channel"));
					// 기본적으로는 송신 OFF
					VoiceChatUser->TransmitToNoChannels();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[Voice] ✗ Failed to join mafia channel: %s"), *Result.ErrorDesc);
				}
			}
		)
	);
}

void ABioPlayerController::LeaveGameChannels()
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser || MafiaGameChannelName.IsEmpty())
		return;

	VoiceChatUser->LeaveChannel(MafiaGameChannelName,
		FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
			[this](const FString& LeftChannel, const FVoiceChatResult& Result)
			{
				if (Result.IsSuccess())
				{
					UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Left mafia channel"));
					MafiaGameChannelName.Empty();
				}
			}
		)
	);
}

void ABioPlayerController::OnVoiceChannelJoined(const FString& ChannelName, const FVoiceChatResult& Result)
{
	if (Result.IsSuccess())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Joined game channel: %s"), *ChannelName);
		VoiceTransmitToPublic();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice] ✗ Failed to join game channel: %s"), *Result.ErrorDesc);
	}
}

// ========================================
// Voice Transmission Control
// ========================================
void ABioPlayerController::EnableMafiaVoice(bool bEnable)
{
	CacheVoiceChatUser();
	if (!VoiceChatUser || MafiaGameChannelName.IsEmpty())
		return;

	if (bEnable)
	{
		TSet<FString> Channels = { MafiaGameChannelName };
		VoiceChatUser->TransmitToSpecificChannels(Channels);
		UE_LOG(LogTemp, Warning, TEXT("[Voice] Mafia voice enabled"));
	}
	else
	{
		VoiceChatUser->TransmitToNoChannels();
		UE_LOG(LogTemp, Warning, TEXT("[Voice] Mafia voice disabled"));
	}
}

void ABioPlayerController::VoiceTransmitToNone()
{
	CacheVoiceChatUser();
	if (VoiceChatUser)
	{
		VoiceChatUser->TransmitToNoChannels();
	}
}

void ABioPlayerController::VoiceTransmitToPublic()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	FString ChannelToUse = PublicGameChannelName.IsEmpty() ? LobbyChannelName : PublicGameChannelName;
	if (ChannelToUse.IsEmpty())
	{
		return;
	}

	TSet<FString> Channels = { ChannelToUse };
	VoiceChatUser->TransmitToSpecificChannels(Channels);
}

void ABioPlayerController::VoiceTransmitToMafiaOnly()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser || MafiaGameChannelName.IsEmpty())
		return;

	TSet<FString> Channels = { MafiaGameChannelName };
	VoiceChatUser->TransmitToSpecificChannels(Channels);
}

void ABioPlayerController::VoiceTransmitToBothChannels()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser || PublicGameChannelName.IsEmpty() || MafiaGameChannelName.IsEmpty())
		return;

	TSet<FString> Channels = { PublicGameChannelName, MafiaGameChannelName };
	VoiceChatUser->TransmitToSpecificChannels(Channels);
}

// ========================================
// Proximity Voice
// ========================================

void ABioPlayerController::StartProximityVoice()
{
	GetWorldTimerManager().SetTimer(ProximityTimer, this, &ABioPlayerController::UpdateProximityVoice, ProxUpdateInterval, true);
}

void ABioPlayerController::UpdateProximityVoice()
{
	if (!IsLocalController())
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	FVector ListenerLoc;
	if (APawn* MyPawn = GetPawn())
	{
		ListenerLoc = MyPawn->GetActorLocation();
	}
	else
	{
		return;
	}

	ABioPlayerState* MyPS = GetPlayerState<ABioPlayerState>();
	const bool bIAmCleaner = (MyPS && MyPS->GameRole == EBioPlayerRole::Cleaner);

	// 모든 플레이어를 순회하면서 거리 기반 볼륨 조절
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* OtherPawn = *It;
		if (!OtherPawn || OtherPawn == GetPawn())
			continue;

		ABioPlayerState* OtherPS = Cast<ABioPlayerState>(OtherPawn->GetPlayerState());
		if (!OtherPS)
			continue;

		const bool bOtherIsCleaner = (OtherPS->GameRole == EBioPlayerRole::Cleaner);

		FVector SpeakerLoc = OtherPawn->GetActorLocation();
		float Distance = FVector::Dist(ListenerLoc, SpeakerLoc);

		float Volume = CalcProxVolume01(Distance, ProxMinDist, ProxMaxDist);

		// 마피아끼리는 항상 최대 볼륨
		if (bIAmCleaner && bOtherIsCleaner)
		{
			Volume = 1.0f;
		}

		// 네이티브 VoIP는 APlayerState를 통해 음량 조절
		//MutePlayer(OtherPS, false); // 음소거 해제
		// 참고: UE5.5에서는 개별 플레이어 볼륨 조절 API가 제한적일 수 있음
		// 필요시 VOIPTalker 컴포넌트를 직접 조작
	}
}

float ABioPlayerController::CalcProxVolume01(float Dist, float MinD, float MaxD)
{
	if (Dist <= MinD) return 1.0f;
	if (Dist >= MaxD) return 0.0f;

	const float Alpha = (Dist - MinD) / (MaxD - MinD);
	return FMath::Clamp(1.0f - (Alpha * Alpha), 0.0f, 1.0f);
}