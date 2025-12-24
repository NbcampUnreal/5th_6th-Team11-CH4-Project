// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/BioProtocolTypes.h"
#include "BioPlayerHUD.generated.h"

class ACharacter;
class UProgressBar;
class UTextBlock;
class UItemSlotWidget;
class UTexture2D;
class UWidgetSwitcher;

UCLASS()
class BIOPROTOCOL_API UBioPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateHealth(float CurrentHP);

	UFUNCTION(BlueprintCallable)
	void UpdateHUDState(EBioPlayerRole Role, EBioGamePhase CurrentPhase, bool bIsTransformed);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateItemSlot(int32 SlotIndex, UTexture2D* Icon);


protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RoleText;

	UPROPERTY(meta = (BindWidget))
	UItemSlotWidget* ItemSlot_0;

	UPROPERTY(meta = (BindWidget))
	UItemSlotWidget* ItemSlot_1;

	UPROPERTY(meta = (BindWidget))
	UItemSlotWidget* ItemSlot_2;

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* RoleSpecificContainer;

	TArray<UItemSlotWidget*> Slots;

	float MaxHP = 100.0f;

	TWeakObjectPtr<ACharacter> OwnerCharacter;
};