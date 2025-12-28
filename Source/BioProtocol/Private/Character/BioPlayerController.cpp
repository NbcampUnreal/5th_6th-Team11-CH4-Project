// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BioPlayerController.h"
#include "UI/BioPlayerHUD.h"
#include "Blueprint/UserWidget.h"

void ABioPlayerController::BeginPlay()
{
	Super::BeginPlay();

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
	}
}
