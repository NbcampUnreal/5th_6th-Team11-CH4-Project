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


void ATestController::Server_CreateSession_Implementation(int32 PublicConnections, bool bIsLAN)
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

	SessionSub->CreateGameSession(PublicConnections, bIsLAN); 
}

