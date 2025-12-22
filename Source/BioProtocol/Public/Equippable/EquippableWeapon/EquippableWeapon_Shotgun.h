// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equippable/EquippableWeapon/EquippableWeapon_.h"
#include "EquippableWeapon_Shotgun.generated.h"

/**
 * 샷건 - 산탄 발사 무기
 * 여러 발의 LineTrace로 근거리 고데미지
 */
UCLASS()
class BIOPROTOCOL_API AEquippableWeapon_Shotgun : public AEquippableWeapon_
{
	GENERATED_BODY()

public:
	AEquippableWeapon_Shotgun();

protected:
	//==========================================
	// SHOTGUN PROPERTIES
	//==========================================

	virtual void ProcessAttack() override;

	/** 산탄 개수 (한 번에 발사되는 탄환 수) */
	UPROPERTY(EditDefaultsOnly, Category = "Shotgun|Settings")
	int32 PelletCount;

	/** 산탄 확산 각도 */
	UPROPERTY(EditDefaultsOnly, Category = "Shotgun|Settings")
	float SpreadAngle;

	/** 히트 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "Shotgun|VFX")
	UParticleSystem* ImpactParticle;
};