// Fill out your copyright notice in the Description page of Project Settings.


#include "TestController.h"
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
#include "VoiceChat.h"





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