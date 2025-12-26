// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/BioPlayerHUD.h"
#include "UI/ItemSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Border.h"

void UBioPlayerHUD::NativeConstruct()
{
	Super::NativeConstruct();

	if (GetOwningPlayerPawn())
	{
		OwnerCharacter = Cast<ACharacter>(GetOwningPlayerPawn());
	}

	Slots.Empty();
	if (ItemSlot_0) Slots.Add(ItemSlot_0);
	if (ItemSlot_1) Slots.Add(ItemSlot_1);
	if (ItemSlot_2) Slots.Add(ItemSlot_2);

	if (RoleSpecificContainer)
	{
		RoleSpecificContainer->SetActiveWidgetIndex(0);
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

void UBioPlayerHUD::UpdateStaminaBar(float NewStamina)
{
	if (!StaminaBar) return;
	
	UE_LOG(LogTemp, Error, TEXT("%f"), NewStamina);

	float Percent = FMath::Clamp(NewStamina / MaxSP, 0.0f, 1.0f);
	StaminaBar->SetPercent(Percent);

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

	if (Role == EBioPlayerRole::Staff || Role == EBioPlayerRole::None || CurrentPhase == EBioGamePhase::Lobby)
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

void UBioPlayerHUD::SelectItemSlot(int32 SlotIndex)
{
	if (SlotHighlight_1) SlotHighlight_1->SetVisibility(ESlateVisibility::Hidden);
	if (SlotHighlight_2) SlotHighlight_2->SetVisibility(ESlateVisibility::Hidden);
	if (SlotHighlight_3) SlotHighlight_3->SetVisibility(ESlateVisibility::Hidden);

	switch (SlotIndex)
	{
	case 1:
		if (SlotHighlight_1) SlotHighlight_1->SetVisibility(ESlateVisibility::Visible);
		break;
	case 2:
		if (SlotHighlight_2) SlotHighlight_2->SetVisibility(ESlateVisibility::Visible);
		break;
	case 3:
		if (SlotHighlight_3) SlotHighlight_3->SetVisibility(ESlateVisibility::Visible);
		break;
	default:
		break;
	}
}

void UBioPlayerHUD::UpdateAmmoText(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (AmmoText)
	{
		FString AmmoStr = FString::Printf(TEXT("%d / %d"), CurrentAmmo, MaxAmmo);
		AmmoText->SetText(FText::FromString(AmmoStr));
	}
}

void UBioPlayerHUD::SetAmmoUIVisibility(bool bIsVisible)
{
	if (AmmoText)
	{
		AmmoText->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}