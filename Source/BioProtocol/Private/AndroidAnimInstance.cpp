// Fill out your copyright notice in the Description page of Project Settings.


#include "AndroidAnimInstance.h"
#include "AndroidCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"

void UAndroidAnimInstance::NativeInitializeAnimation()
{
	OwnerCharacter = Cast<AAndroidCharacter>(GetOwningActor());
	if (IsValid(OwnerCharacter) == true)
	{
		OwnerCharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	}
}

void UAndroidAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (IsValid(OwnerCharacter) == false || IsValid(OwnerCharacterMovementComponent) == false)
	{
		return;
	}

	Velocity = OwnerCharacterMovementComponent->Velocity;
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	bShouldMove = ((OwnerCharacterMovementComponent->GetCurrentAcceleration().IsNearlyZero()) == false) && (3.f < GroundSpeed);

	bIsRunning = (bShouldMove == true) && (900.f < GroundSpeed);

	bIsFalling = OwnerCharacterMovementComponent->IsFalling();

	bIsCrouch = OwnerCharacterMovementComponent->IsCrouching();
}
