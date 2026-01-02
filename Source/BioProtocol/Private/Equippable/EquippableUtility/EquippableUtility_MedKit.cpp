// Fill out your copyright notice in the Description page of Project Settings.


#include "Equippable/EquippableUtility/EquippableUtility_MedKit.h"
#include "BioProtocol/Public/Character/StaffCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

AEquippableUtility_MedKit::AEquippableUtility_MedKit()
{
	bConsumable = true;
	MaxUses = 1;
	Cooldown = 0.0f;

	HealAmount = 50.0f;
	bCanHealOthers = true;
	HealRange = 200.0f;  // 2미터
}

void AEquippableUtility_MedKit::ProcessUtilityEffect()
{
	if (!OwningCharacter)
	{
		return;
	}

	// 타인 치료 시도
	if (bCanHealOthers)
	{
		AStaffCharacter* Target = FindHealTarget();
		if (Target && Target != OwningCharacter)
		{
			// 타인 치료
			HealOther();
			return;
		}
	}

	// 자가 치료
	HealSelf();
}

void AEquippableUtility_MedKit::HealSelf()
{
	if (!OwningCharacter || !HasAuthority())
	{
		return;
	}

	// 현재 체력
	float CurrentHealth = OwningCharacter->GetHealth();
	float MaxHealth = OwningCharacter->GetMaxHealth();

	// 이미 풀피면 사용 안함
	if (CurrentHealth >= MaxHealth)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MedKit] Already at full health"));
		return;
	}

	// 체력 회복
	float NewHealth = FMath::Min(CurrentHealth + HealAmount, MaxHealth);
	OwningCharacter->SetHealth(NewHealth);

	UE_LOG(LogTemp, Log, TEXT("[MedKit] Self heal: %.1f -> %.1f (+ %.1f)"),
		CurrentHealth, NewHealth, HealAmount);
}

void AEquippableUtility_MedKit::HealOther()
{
	AStaffCharacter* Target = FindHealTarget();
	if (!Target || !HasAuthority())
	{
		return;
	}

	// 타겟 체력 회복
	float CurrentHealth = Target->GetHealth();
	float MaxHealth = Target->GetMaxHealth();

	if (CurrentHealth >= MaxHealth)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MedKit] Target already at full health"));
		return;
	}

	float NewHealth = FMath::Min(CurrentHealth + HealAmount, MaxHealth);
	Target->SetHealth(NewHealth);

	UE_LOG(LogTemp, Log, TEXT("[MedKit] Healed %s: %.1f -> %.1f (+ %.1f)"),
		*Target->GetName(), CurrentHealth, NewHealth, HealAmount);
}

AStaffCharacter* AEquippableUtility_MedKit::FindHealTarget()
{
	if (!OwningCharacter)
	{
		return nullptr;
	}

	// 카메라 방향으로 LineTrace
	UCameraComponent* Camera = OwningCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return nullptr;
	}

	FVector Start = Camera->GetComponentLocation();
	FVector Forward = Camera->GetForwardVector();
	FVector End = Start + (Forward * HealRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwningCharacter);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Pawn,
		QueryParams
	);

	if (bHit)
	{
		AStaffCharacter* Target = Cast<AStaffCharacter>(HitResult.GetActor());
		if (Target)
		{
			// 타겟이 아군인지 확인 (팀 체크 로직 필요)
			return Target;
		}
	}

	return nullptr;
}