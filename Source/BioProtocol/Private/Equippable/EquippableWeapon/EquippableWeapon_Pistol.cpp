// Fill out your copyright notice in the Description page of Project Settings.


#include "Equippable/EquippableWeapon/EquippableWeapon_Pistol.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AEquippableWeapon_Pistol::AEquippableWeapon_Pistol()
{
	// 권총 스탯
	Damage = 25.0f;
	Range = 5000.0f;  // 50미터
	FireRate = 3.0f;   // 초당 3발
	bIsMeleeWeapon = false;
	bInfiniteAmmo = false;
	MaxAmmo = 12;

	MuzzleSocketName = FName("Muzzle");
	ImpactParticle = nullptr;
}

void AEquippableWeapon_Pistol::ProcessAttack()
{
	if (!OwningCharacter)
	{
		return;
	}

	// 카메라 기준으로 조준
	UCameraComponent* Camera = OwningCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return;
	}

	// LineTrace 시작/끝 위치
	//FVector Start = Camera->GetComponentLocation();
	FVector Start = OwningCharacter->GetMesh()->GetSocketLocation(MuzzleSocketName);
	FVector Forward = Camera->GetForwardVector();
	FVector End = Start + (Forward * Range);

	// LineTrace 실행
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwningCharacter);
	QueryParams.bTraceComplex = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	// 디버그 라인
	DrawDebugLine(
		GetWorld(),
		Start,
		bHit ? HitResult.Location : End,
		bHit ? FColor::Red : FColor::Green,
		false,
		1.0f,
		0,
		1.0f
	);

	if (bHit)
	{
		// 데미지 적용 (서버에서만)
		if (HasAuthority())
		{
			if (AActor* HitActor = HitResult.GetActor())
			{
				UGameplayStatics::ApplyPointDamage(
					HitActor,
					Damage,
					Forward,
					HitResult,
					OwningCharacter->GetController(),
					this,
					UDamageType::StaticClass()
				);

				UE_LOG(LogTemp, Log, TEXT("[Pistol] Hit: %s for %.1f damage"),
					*HitActor->GetName(), Damage);
			}
		}

		// 히트 이펙트
		if (ImpactParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticle,
				HitResult.Location,
				HitResult.Normal.Rotation()
			);
		}

		ProcessHit(HitResult);
	}
}