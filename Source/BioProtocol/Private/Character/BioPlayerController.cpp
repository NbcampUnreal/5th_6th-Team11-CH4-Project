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

ABioPlayerController::ABioPlayerController()
{
	bReplicates = true;
	UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerController Constructor - bReplicates: %s"),
		bReplicates ? TEXT("TRUE") : TEXT("FALSE"));
}

void ABioPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerController BeginPlay - Authority: %s, LocalController: %s"),
		HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
		IsLocalController() ? TEXT("YES") : TEXT("NO"));

	// UI 초기화 (로컬 컨트롤러만)
	if (IsLocalController())
	{
		if (BioHUDClass)
		{
			InGameHUD = CreateWidget<UBioPlayerHUD>(this, BioHUDClass);

			if (InGameHUD)
			{
				InGameHUD->AddToViewport();
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("BioHUDClass is NOT set in BioPlayerController!"));
		}

		// 근접 보이스 시작
		StartProximityVoice();
	}

	// PlayerState 확인
	ABioPlayerState* PS = GetPlayerState<ABioPlayerState>();
	if (PS)
	{
		UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerState Found: %s, Role: %d"),
			*PS->GetName(), (int32)PS->GameRole);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerState NOT FOUND - Will retry"));

		FTimerHandle RetryTimer;
		GetWorldTimerManager().SetTimer(RetryTimer, [this]()
			{
				ABioPlayerState* RetryPS = GetPlayerState<ABioPlayerState>();
				if (RetryPS)
				{
					UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerState Found (Retry): %s"), *RetryPS->GetName());
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("🔧 BioPlayerState STILL NOT FOUND!"));
				}
			}, 1.0f, false);
	}

	// 게임 맵에서는 송신 중지 상태로 시작
	UWorld* World = GetWorld();
	if (World)
	{
		FString MapName = World->GetMapName();
		if (MapName.StartsWith(TEXT("UEDPIE_")))
		{
			int32 UnderscoreIndex;
			if (MapName.FindChar('_', UnderscoreIndex))
			{
				MapName = MapName.RightChop(UnderscoreIndex + 1);
			}
		}

		if (MapName.Contains(TEXT("/")))
		{
			int32 LastSlashIndex;
			MapName.FindLastChar('/', LastSlashIndex);
			MapName = MapName.RightChop(LastSlashIndex + 1);
		}

		if (MapName.Contains(TEXT("MainLevel")) || MapName.Contains(TEXT("GameMap")))
		{
			ClientShowLoadingScreen();
			VoiceTransmitToNone();
		}

		//if (IsLocalController() && MapName.Contains(TEXT("Lobby")))
		//{
		//	VoiceTransmitToALL();
		//}
	}
}

void ABioPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ProximityTimer);
	Super::EndPlay(EndPlayReason);
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
					VoiceTransmitToLobby();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[Voice] ✗ Failed to join lobby: %s"), *Result.ErrorDesc);
				}
			}
		)
	);
}

void ABioPlayerController::Client_LeaveGameChannels_Implementation()
{
	LeaveGameChannels();
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

	bool bIsMafiaChannel = ChannelName.Contains(TEXT("Mafia"));
	bool bIsPublicChannel = ChannelName.Contains(TEXT("Citizen")) || ChannelName.Contains(TEXT("Public"));

	EVoiceChatChannelType ChannelType = EVoiceChatChannelType::NonPositional;

	if (bIsMafiaChannel)
	{
		MafiaGameChannelName = ChannelName;
		ChannelType = EVoiceChatChannelType::NonPositional;
	}
	else if (bIsPublicChannel)
	{
		PublicGameChannelName = ChannelName;
		ChannelType = EVoiceChatChannelType::Positional;
	}
	else
	{
		return;
	}

	FEOSVoiceChatChannelCredentials Creds;
	Creds.ClientBaseUrl = ClientBaseUrl;
	Creds.ParticipantToken = ParticipantToken;

	VoiceChatUser->JoinChannel(ChannelName, Creds.ToJson(), ChannelType,
		FOnVoiceChatChannelJoinCompleteDelegate::CreateUObject(this, &ABioPlayerController::OnVoiceChannelJoined)
	);
}

void ABioPlayerController::LeaveGameChannels()
{
	if (!IsLocalController())
		return;

	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	if (!PublicGameChannelName.IsEmpty())
	{
		VoiceChatUser->LeaveChannel(PublicGameChannelName,
			FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
				[this](const FString&, const FVoiceChatResult& Result)
				{
					if (Result.IsSuccess())
					{
						UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Left public game channel"));
						PublicGameChannelName.Empty();
					}
				}
			)
		);
	}

	if (!MafiaGameChannelName.IsEmpty())
	{
		VoiceChatUser->LeaveChannel(MafiaGameChannelName,
			FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
				[this](const FString&, const FVoiceChatResult& Result)
				{
					if (Result.IsSuccess())
					{
						UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Left mafia game channel"));
						MafiaGameChannelName.Empty();
					}
				}
			)
		);
	}
}

void ABioPlayerController::OnVoiceChannelJoined(const FString& ChannelName, const FVoiceChatResult& Result)
{
	if (Result.IsSuccess())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Joined game channel: %s"), *ChannelName);
		//VoiceTransmitToPublic();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice] ✗ Failed to join game channel: %s"), *Result.ErrorDesc);
	}
}

// ========================================
// Voice Transmission Control
// ========================================

void ABioPlayerController::VoiceTransmitToNone()
{
	CacheVoiceChatUser();
	if (VoiceChatUser)
	{
		VoiceChatUser->TransmitToNoChannels();
	}
}

void ABioPlayerController::VoiceTransmitToALL()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	VoiceChatUser->TransmitToAllChannels();
}

void ABioPlayerController::VoiceTransmitToLobby()
{
	CacheVoiceChatUser();
	if (!VoiceChatUser)
		return;

	FString ChannelToUse = LobbyChannelName;
	UE_LOG(LogTemp, Log, TEXT("[Voice] LobbyChannelName : %s"), *LobbyChannelName);
	if (ChannelToUse.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[Voice] ChannelToUse.IsEmpty()"));
		return;
	}

	TSet<FString> Channels = { ChannelToUse };
	VoiceChatUser->TransmitToSpecificChannels(Channels);
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

	CacheVoiceChatUser();
	if (!VoiceChatUser || PublicGameChannelName.IsEmpty())
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

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

	ABioPlayerState* MyPS = GetPlayerState<ABioPlayerState>();
	const bool bIAmCleaner = (MyPS && MyPS->GameRole == EBioPlayerRole::Cleaner);

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* OtherPawn = *It;
		if (!OtherPawn || OtherPawn == GetPawn())
			continue;

		ABioPlayerState* OtherPS = Cast<ABioPlayerState>(OtherPawn->GetPlayerState());
		if (!OtherPS || OtherPS->EOSPlayerName.IsEmpty())
			continue;

		const bool bOtherIsCleaner = (OtherPS->GameRole == EBioPlayerRole::Cleaner);

		FVector SpeakerLoc = OtherPawn->GetActorLocation();
		float Distance = FVector::Dist(ListenerLoc, SpeakerLoc);

		float Volume = CalcProxVolume01(Distance, ProxMinDist, ProxMaxDist);

		//VoiceChatUser->Set3DPosition(PublicGameChannelName, SpeakerLoc, ListenerLoc, ListenerForward, ListenerUp);

		// 마피아끼리는 항상 최대 볼륨
		if (bIAmCleaner && bOtherIsCleaner)
		{
			Volume = 1.0f;
		}

		VoiceChatUser->SetPlayerVolume(OtherPS->EOSPlayerName, Volume);
	}
}

float ABioPlayerController::CalcProxVolume01(float Dist, float MinD, float MaxD)
{
	if (Dist <= MinD) return 1.0f;
	if (Dist >= MaxD) return 0.0f;

	const float Alpha = (Dist - MinD) / (MaxD - MinD);
	return FMath::Clamp(1.0f - (Alpha * Alpha), 0.0f, 1.0f);
}

// ========================================
// Loading UI
// ========================================

void ABioPlayerController::ClientShowLoadingScreen_Implementation()
{
	if (!LoadingScreenWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadingScreenWidgetClass is not set!"));
		return;
	}

	if (!LoadingScreenWidget)
	{
		LoadingScreenWidget = CreateWidget<UUserWidget>(this, LoadingScreenWidgetClass);
	}

	if (LoadingScreenWidget && !LoadingScreenWidget->IsInViewport())
	{
		LoadingScreenWidget->AddToViewport(999);

		// 입력 비활성화
		DisableInput(this);


		UE_LOG(LogTemp, Log, TEXT("Loading screen shown, input disabled"));
	}
}

void ABioPlayerController::ClientHideLoadingScreen_Implementation()
{
	if (LoadingScreenWidget && LoadingScreenWidget->IsInViewport())
	{
		LoadingScreenWidget->RemoveFromParent();

		// 입력 다시 활성화
		EnableInput(this);

		UE_LOG(LogTemp, Log, TEXT("Loading screen hidden, input enabled"));
	}
}