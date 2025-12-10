// Fill out your copyright notice in the Description page of Project Settings.


#include "TestController.h"
#include "../Session/SessionSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"





void ATestController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (IsValid(SessionWidgetClass) == true)
	{
		SessionWidgetInstance = CreateWidget<UUserWidget>(this, SessionWidgetClass);
		if (IsValid(SessionWidgetInstance) == true)
		{
			SessionWidgetInstance->AddToViewport();
		}
	}

	
}


void ATestController::HostGame()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USessionSubsystem* SessionSubsystem = GI->GetSubsystem<USessionSubsystem>())
		{
			SessionSubsystem->CreateGameSession(4, true);
		}
	}
}

void ATestController::FindGames()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USessionSubsystem* SessionSubsystem = GI->GetSubsystem<USessionSubsystem>())
		{
			SessionSubsystem->FindGameSessions(100, true);
		}
	}
}

void ATestController::JoinGame()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USessionSubsystem* SessionSubsystem = GI->GetSubsystem<USessionSubsystem>())
		{
			SessionSubsystem->JoinFoundSession();
		}
	}
}