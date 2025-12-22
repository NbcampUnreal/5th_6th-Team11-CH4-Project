// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equippable/EquippableItem.h"
#include "EquippableTool_Wrench.generated.h"

class ADXPlayerCharacter;
/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API AEquippableTool_Wrench : public AEquippableItem
{
	GENERATED_BODY()

public:
	AEquippableTool_Wrench();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Use() override;
	virtual void StopUsing() override;
	virtual bool CanUse() const override;

	virtual void PlayUseAnimation() override;

protected:

	// 렌치 사용 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Wrench|Audio")
	USoundBase* WrenchUseSound;

	// 렌치 사용 파티클
	UPROPERTY(EditDefaultsOnly, Category = "Wrench|VFX")
	UParticleSystem* WrenchUseParticle;

	/*
	UPROPERTY()
	ADXPlayerCharacter* OwningCharacter; */

	// 렌치 사용 애니메이션 몽타주 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Wrench|Animation")
	UAnimMontage* WrenchUseMontage;
};
