// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "StaffAnimInstance.generated.h"

class AStaffCharacter;
class UCharacterMovementComponent;
/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API UStaffAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
protected:
	UPROPERTY()
	TObjectPtr<AStaffCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> OwnerCharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector Velocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float GroundSpeed;	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	uint8 bShouldMove : 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	uint8 bIsFalling : 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	uint8 bIsRunning : 1;	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bIsCrouch : 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bIsGun:1;
};
