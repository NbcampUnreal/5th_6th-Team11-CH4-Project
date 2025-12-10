// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/BioPlayerHUD.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"

void UBioPlayerHUD::NativeConstruct()
{
	Super::NativeConstruct();

	if (GetOwningPlayerPawn())
	{
		OwnerCharacter = Cast<ACharacter>(GetOwningPlayerPawn());
	}
}

void UBioPlayerHUD::UpdateHealth(float CurrentHP)
{
	if (HealthBar)
	{
		float Percent = FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);
		HealthBar->SetPercent(Percent);
	}
	if (HealthText)
	{
		FText Health = FText::AsNumber((int32)CurrentHP);
		HealthText->SetText(Health);
	}
}

void UBioPlayerHUD::UpdateRoleText(FString NewRole)
{
	if (RoleText)
	{
		RoleText->SetText(FText::FromString(NewRole));
	}
}