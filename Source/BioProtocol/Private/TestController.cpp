// Fill out your copyright notice in the Description page of Project Settings.


#include "TestController.h"
#include "../Session/SessionSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"





void ATestController::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(CreateSessionWidgetClass) == true)
	{
		CreateSessionWidgetInstance = CreateWidget<UUserWidget>(this, CreateSessionWidgetClass);
		if (IsValid(CreateSessionWidgetInstance) == true)
		{
			CreateSessionWidgetInstance->AddToViewport();
		}
	}

	if (IsValid(JoinSessionWidgetClass) == true)
	{
		JoinSessionWidgetInstance = CreateWidget<UUserWidget>(this, JoinSessionWidgetClass);
		if (IsValid(JoinSessionWidgetInstance) == true)
		{
			JoinSessionWidgetInstance->AddToViewport();
		}
	}

	if (IsValid(FindSessionWidgetClass) == true)
	{
		FindSessionWidgetInstance = CreateWidget<UUserWidget>(this, FindSessionWidgetClass);
		if (IsValid(FindSessionWidgetInstance) == true)
		{
			FindSessionWidgetInstance->AddToViewport();
		}
	}
}
