// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/BioPlayerHUD.h"
#include "UI/ItemSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/WidgetSwitcher.h"
#include "Game/BioGameState.h"
#include "Character/StaffStatusComponent.h"

void UBioPlayerHUD::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (OwningPawn)
	{
		UStaffStatusComponent* StatusComp = OwningPawn->FindComponentByClass<UStaffStatusComponent>();
		if (StatusComp)
		{
			CachedStatusComp = StatusComp;

			StatusComp->OnHPChanged.AddDynamic(this, &UBioPlayerHUD::UpdateHP);
			StatusComp->OnStaminaChanged.AddDynamic(this, &UBioPlayerHUD::UpdateStamina);

			UpdateHP(StatusComp->CurrentHP);
			UpdateStamina(StatusComp->CurrentStamina);
		}
	}

	UWorld* World = GetWorld();
	if (World)
	{
		CachedGameState = World->GetGameState<ABioGameState>();
		if (CachedGameState.IsValid())
		{
			CachedGameState->OnPhaseChanged.AddDynamic(this, &UBioPlayerHUD::UpdateGamePhase);

			UpdateGamePhase(CachedGameState->CurrentPhase);
		}
	}

	if (EscapeNotifyText) EscapeNotifyText->SetVisibility(ESlateVisibility::Hidden);

	Slots.Empty();
	if (ItemSlot_0) Slots.Add(ItemSlot_0);
	if (ItemSlot_1) Slots.Add(ItemSlot_1);
	if (ItemSlot_2) Slots.Add(ItemSlot_2);

	if (RoleSpecificContainer)
	{
		RoleSpecificContainer->SetActiveWidgetIndex(0);
	}
}

void UBioPlayerHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateGameStateInfo();
}

void UBioPlayerHUD::UpdateHP(float NewHP)
{
	if (HealthBar && CachedStatusComp.IsValid())
	{
		float MaxHP = CachedStatusComp->MaxHP;
		float Percent = (MaxHP > 0.f) ? (NewHP / MaxHP) : 0.f;
		HealthBar->SetPercent(Percent);
	}
}

void UBioPlayerHUD::UpdateStamina(float NewStamina)
{
	if (StaminaBar && CachedStatusComp.IsValid())
	{
		float MaxStamina = CachedStatusComp->MaxStamina;
		float Percent = (MaxStamina > 0.f) ? (NewStamina / MaxStamina) : 0.f;
		StaminaBar->SetPercent(Percent);
	}
}

void UBioPlayerHUD::UpdateGamePhase(EBioGamePhase NewPhase)
{
	if (!PhaseText) return;

	FString PhaseString;
	FSlateColor PhaseColor = FLinearColor::White;

	switch (NewPhase)
	{
	case EBioGamePhase::Day:
		PhaseString = TEXT("업무시간");
		PhaseColor = FLinearColor::Yellow;
		break;
	case EBioGamePhase::Night:
		PhaseString = TEXT("청소시간");
		PhaseColor = FLinearColor::Red;
		break;
	case EBioGamePhase::End:
		PhaseString = TEXT("게임 종료");
		break;
	}

	PhaseText->SetText(FText::FromString(PhaseString));
	PhaseText->SetColorAndOpacity(PhaseColor);
}

void UBioPlayerHUD::UpdateGameStateInfo()
{
	if (!CachedGameState.IsValid())
	{
		CachedGameState = GetWorld()->GetGameState<ABioGameState>();
		return;
	}

	if (TimeText)
	{
		int32 RemainingTime = CachedGameState->PhaseTimeRemaining;
		int32 Minutes = RemainingTime / 60;
		int32 Seconds = RemainingTime % 60;

		FString TimeStr = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		TimeText->SetText(FText::FromString(TimeStr));
	}

	if (MissionProgressBar)
	{
		float MaxMission = (float)CachedGameState->MaxMissionProgress;
		float CurrentMission = (float)CachedGameState->MissionProgress;

		MissionProgressBar->SetPercent(CurrentMission / MaxMission);
	}

	if (EscapeNotifyText)
	{
		if (CachedGameState->bCanEscape)
		{
			if (EscapeNotifyText->GetVisibility() != ESlateVisibility::Visible)
			{
				EscapeNotifyText->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}

void UBioPlayerHUD::UpdateItemSlot(int32 SlotIndex, UTexture2D* Icon)
{
	if (Slots.IsValidIndex(SlotIndex) && Slots[SlotIndex])
	{
		Slots[SlotIndex]->UpdateSlot(Icon);
	}
}

void UBioPlayerHUD::UpdateHUDState(EBioPlayerRole Role, EBioGamePhase CurrentPhase, bool bIsTransformed)
{
	if (!RoleSpecificContainer) return;

	if (Role == EBioPlayerRole::Staff || Role == EBioPlayerRole::None)
	{
		RoleSpecificContainer->SetActiveWidgetIndex(0);
		return;
	}

	if (Role == EBioPlayerRole::Cleaner)
	{
		if (CurrentPhase == EBioGamePhase::Day)
		{
			RoleSpecificContainer->SetActiveWidgetIndex(1);
		}
		else if (CurrentPhase == EBioGamePhase::Night)
		{
			if (bIsTransformed)
			{
				RoleSpecificContainer->SetActiveWidgetIndex(3);
			}
			else
			{
				RoleSpecificContainer->SetActiveWidgetIndex(2);
			}
		}
		else
		{
			RoleSpecificContainer->SetActiveWidgetIndex(0);
		}
	}
}