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
class UBorder;

UCLASS()
class BIOPROTOCOL_API UBioPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateHealth(float CurrentHP);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateStaminaBar(float NewStamina);

	UFUNCTION(BlueprintCallable)
	void UpdateHUDState(EBioPlayerRole Role, EBioGamePhase CurrentPhase, bool bIsTransformed);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateItemSlot(int32 SlotIndex, UTexture2D* Icon);

	UFUNCTION(BlueprintCallable)
	void SelectItemSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateAmmoText(int32 CurrentAmmo, int32 MaxAmmo);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAmmoUIVisibility(bool bIsVisible);


protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaBar;

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
	float MaxSP = 100.0f;

	TWeakObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(meta = (BindWidget))
	UBorder* SlotHighlight_1;

	UPROPERTY(meta = (BindWidget))
	UBorder* SlotHighlight_2;

	UPROPERTY(meta = (BindWidget))
	UBorder* SlotHighlight_3;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoText;
};