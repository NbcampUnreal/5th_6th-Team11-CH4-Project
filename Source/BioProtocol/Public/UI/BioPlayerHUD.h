// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BioPlayerHUD.generated.h"

class ACharacter;
class UProgressBar;
class UTextBlock;

UCLASS()
class BIOPROTOCOL_API UBioPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateHealth(float CurrentHP);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateRoleText(FString NewRole);

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RoleText;

	float MaxHP = 100.0f;

	TWeakObjectPtr<ACharacter> OwnerCharacter;
};