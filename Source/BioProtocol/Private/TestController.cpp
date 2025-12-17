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






void ATestController::BeginPlay()
{
	Super::BeginPlay();

	//FOnlineAccountCredentials Creds;
	//Creds.Type = TEXT("developer");          // DevAuthTool 방식
	//Creds.Id = TEXT("localhost:8081");     // DevAuthTool에서 쓴 포트
	//Creds.Token = TEXT("DevUser1");         // DevAuthTool에서 만든 Credential 이름

}

void ATestController::LogintoEOS(int32 Credential)
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

	Identity->AddOnLoginCompleteDelegate_Handle(LocalUserNum, FOnLoginCompleteDelegate::CreateUObject(this, &ATestController::HandleLoginComplete));

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

