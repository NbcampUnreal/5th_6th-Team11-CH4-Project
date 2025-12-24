// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillSlotWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class BIOPROTOCOL_API USkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void StartCooldown(float Duration);

protected:

	UPROPERTY(meta = (BindWidget))
	UImage* SkillIcon;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* CooldownBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CooldownText;

private:
	bool bIsCoolingDown = false;
	float TotalCooldownTime = 0.0f;
	float CurrentCooldownTime = 0.0f;

	void EndCooldown();
};