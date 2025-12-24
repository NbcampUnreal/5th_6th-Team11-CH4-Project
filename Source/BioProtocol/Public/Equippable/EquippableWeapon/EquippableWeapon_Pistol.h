// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equippable/EquippableWeapon/EquippableWeapon_.h"
#include "EquippableWeapon_Pistol.generated.h"

 /**
  * 권총 - 원거리 단발 사격 무기
  * LineTrace 히트스캔 방식
  */
UCLASS()
class BIOPROTOCOL_API AEquippableWeapon_Pistol : public AEquippableWeapon_
{
	GENERATED_BODY()

public:
	AEquippableWeapon_Pistol();

protected:
	//==========================================
	// SHOOTING
	//==========================================

	virtual void ProcessAttack() override;

	/** 히트 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "Pistol|VFX")
	UParticleSystem* ImpactParticle;

	/** 총구 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Pistol|Settings")
	FName MuzzleSocketName;
};