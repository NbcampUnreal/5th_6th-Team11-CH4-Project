// Fill out your copyright notice in the Description page of Project Settings.


#include "TestController.h"
#include "../Session/SessionSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "TESTPlayerState.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"






void ATestController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!IsLocalController())
		return;

	if (IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld()))
	{
		IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
		if (Identity.IsValid())
		{
			const int32 LocalUserNum = 0;

			// 이미 로그인 상태면 스킵
			if (Identity->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn)
				return;

			FOnlineAccountCredentials Creds;
			Creds.Type = TEXT("developer");          // DevAuthTool 방식
			Creds.Id = TEXT("localhost:8081");     // DevAuthTool에서 쓴 포트
			Creds.Token = TEXT("DevUser1");         // DevAuthTool에서 만든 Credential 이름

			Identity->AddOnLoginCompleteDelegate_Handle(0,
				FOnLoginCompleteDelegate::CreateUObject(this, &ATestController::HandleLoginComplete));

			Identity->Login(0, Creds);
		}

	}

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

