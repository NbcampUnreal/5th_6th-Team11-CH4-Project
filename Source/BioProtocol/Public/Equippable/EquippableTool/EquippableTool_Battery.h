// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "EquippableTool_Battery.generated.h"


/**
 * 배터리 - 무거운 업무용 도구
 * 특징:
 * - 고난이도 임무에 사용
 * - 아이템 슬롯에 넣지 못하고 양손으로 들어야 함
 * - 이동속도 감소 (기본 대비 50%)
 * - 다른 아이템 사용 불가 (내려놓아야 함)
 */

UCLASS()
class BIOPROTOCOL_API AEquippableTool_Battery : public AEquippableItem
{
	GENERATED_BODY()

public:
	AEquippableTool_Battery();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	virtual void Use() override;
	virtual bool CanUse() const override;
	virtual void Equip() override;
	virtual void Unequip() override;

protected:
	// 배터리 무게로 인한 이동속도 감소율 (0.0 ~ 1.0)
	UPROPERTY(EditDefaultsOnly, Category = "Battery|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MovementSpeedModifier;

	// 원래 이동 속도 (복원용)
	UPROPERTY()
	float OriginalMaxWalkSpeed;

	// 배터리 들고 있는 상태인지
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Battery")
	bool bIsCarrying;

	// 이동속도 적용
	UFUNCTION()
	void ApplyMovementPenalty();

	// 이동속도 복구
	UFUNCTION()
	void RemoveMovementPenalty();

	// RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerApplyMovementPenalty();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRemoveMovementPenalty();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
