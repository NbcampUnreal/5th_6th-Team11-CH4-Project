// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/BioProtocolTypes.h"
#include "Daeho/TestCharacter.h"
#include "BioPlayerHUD.generated.h"

class ACharacter;
class UProgressBar;
class UTextBlock;
class UItemSlotWidget;
class UTexture2D;
class UImage;
class UWidgetSwitcher;
class UStaffStatusComponent;
class ABioGameState;
class ABioPlayerState;

UCLASS()
class BIOPROTOCOL_API UBioPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void UpdateHUDState(EBioPlayerRole Role, EBioGamePhase CurrentPhase, bool bIsTransformed);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateItemSlot(int32 SlotIndex, UTexture2D* Icon);

	UFUNCTION(BlueprintCallable)
	void BindStaffStatusComponent();

	UFUNCTION(BlueprintCallable)
	void BindPlayerState();

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

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PhaseText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TimeText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* MissionProgressBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* EscapeNotifyText;

	TArray<UItemSlotWidget*> Slots;

	TWeakObjectPtr<ACharacter> OwnerCharacter;

	UFUNCTION()
	void UpdateHP(float NewHP);

	UFUNCTION()
	void UpdateStamina(float NewStamina);

	UFUNCTION()
	void UpdateGamePhase(EBioGamePhase NewPhase);

	void UpdateGameStateInfo();

	UPROPERTY(meta = (BindWidget))
	UImage* PlayerColorImage;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Color")
	TArray<FLinearColor> PlayerColors;

	UFUNCTION()
	void UpdatePlayerColor(int32 NewColorIndex);

private:
	TWeakObjectPtr<UStaffStatusComponent> CachedStatusComp;

	TWeakObjectPtr<ABioGameState> CachedGameState;

	TWeakObjectPtr<ABioPlayerState> CachedPlayerState;
};