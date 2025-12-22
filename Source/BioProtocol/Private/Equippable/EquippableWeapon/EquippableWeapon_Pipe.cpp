// Fill out your copyright notice in the Description page of Project Settings.


#include "Equippable/EquippableWeapon/EquippableWeapon_Pipe.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

AEquippableWeapon_Pipe::AEquippableWeapon_Pipe()
{
	// 쇠파이프 스탯
	Damage = 30.0f;
	Range = 200.0f;  // 2미터
	FireRate = 1.0f;  // 초당 1회 공격
	bIsMeleeWeapon = true;
	bInfiniteAmmo = true;

	// 근접 공격 박스 크기
	MeleeBoxExtent = FVector(50.0f, 50.0f, 50.0f);
}

void AEquippableWeapon_Pipe::ProcessAttack()
{
	if (!OwningCharacter)
	{
		return;
	}

	// 공격 시작 위치 (캐릭터 앞)
	FVector Start = OwningCharacter->GetActorLocation();
	FVector Forward = OwningCharacter->GetActorForwardVector();
	FVector End = Start + (Forward * Range);

	// BoxTrace 실행
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(OwningCharacter);

	TArray<FHitResult> HitResults;
	bool bHit = UKismetSystemLibrary::BoxTraceMulti(
		GetWorld(),
		Start,
		End,
		MeleeBoxExtent,
		OwningCharacter->GetActorRotation(),
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		1.0f
	);

	if (bHit)
	{
		for (const FHitResult& HitResult : HitResults)
		{
			if (AActor* HitActor = HitResult.GetActor())
			{
				// 데미지 적용
				if (HasAuthority())
				{
					UGameplayStatics::ApplyDamage(
						HitActor,
						Damage,
						OwningCharacter->GetController(),
						this,
						UDamageType::StaticClass()
					);

					UE_LOG(LogTemp, Log, TEXT("[Pipe] Hit: %s for %.1f damage"),
						*HitActor->GetName(), Damage);
				}

				// 히트 이펙트 처리
				ProcessHit(HitResult);
			}
		}
	}

	// 공격 애니메이션
	PlayMeleeAnimation();
}

void AEquippableWeapon_Pipe::PlayMeleeAnimation()
{
	if (!OwningCharacter || !PipeMeleeMontage)
		return;

	OwningCharacter->PlayMeleeMontage(PipeMeleeMontage);
}