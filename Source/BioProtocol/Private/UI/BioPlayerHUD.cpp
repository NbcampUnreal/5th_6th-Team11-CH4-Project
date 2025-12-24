// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/BioPlayerHUD.h"
#include "UI/ItemSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Character/StaffStatusComponent.h"

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

	UStaffStatusComponent* StatusComp =
		OwnerCharacter->FindComponentByClass<UStaffStatusComponent>();

	if (StatusComp)
	{
	//스테미나추가필요
		StatusComp->OnHPChanged.AddDynamic(
			this, &UBioPlayerHUD::UpdateHealth);

		UpdateHealth(StatusComp->GetCurrentHP());
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

void UBioPlayerHUD::UpdateItemSlot(int32 SlotIndex, UTexture2D* Icon)
{
	if (Slots.IsValidIndex(SlotIndex) && Slots[SlotIndex])
	{
		Slots[SlotIndex]->UpdateSlot(Icon);
	}
}