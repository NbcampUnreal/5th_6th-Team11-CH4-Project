
#include "Equippable/EquippableWeapon/EquippableWeapon_Shotgun.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

AEquippableWeapon_Shotgun::AEquippableWeapon_Shotgun()
{
	// 샷건 스탯
	Damage = 80.0f;    // 높은 데미지
	Range = 1000.0f;   // 10미터 (짧은 사거리)
	FireRate = 1.0f;   // 초당 1발
	bIsMeleeWeapon = false;
	bInfiniteAmmo = false;
	MaxAmmo = 6;

	// 산탄 설정
	PelletCount = 8;      // 8발의 산탄
	SpreadAngle = 5.0f;   // 5도 확산

	ImpactParticle = nullptr;
}

void AEquippableWeapon_Shotgun::ProcessAttack()
{
	if (!OwningCharacter)
	{
		return;
	}

	UCameraComponent* Camera = OwningCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return;
	}

	FVector Start = Camera->GetComponentLocation();
	FVector Forward = Camera->GetForwardVector();

	// 산탄별로 LineTrace 실행
	TSet<AActor*> HitActors;  // 중복 데미지 방지

	for (int32 i = 0; i < PelletCount; ++i)
	{
		// 랜덤 확산 적용
		FVector RandomSpread = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(
			Forward,
			SpreadAngle
		);

		FVector End = Start + (RandomSpread * Range);

		// LineTrace
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
			bHit ? FColor::Red : FColor::Orange,
			false,
			0.5f,
			0,
			0.5f
		);

		if (bHit)
		{
			AActor* HitActor = HitResult.GetActor();

			// 데미지 적용 (서버에서만, 중복 방지)
			if (HasAuthority() && HitActor && !HitActors.Contains(HitActor))
			{
				HitActors.Add(HitActor);

				// 각 산탄은 전체 데미지를 나눠서 적용
				float PelletDamage = Damage / static_cast<float>(PelletCount);

				UGameplayStatics::ApplyPointDamage(
					HitActor,
					PelletDamage,
					RandomSpread,
					HitResult,
					OwningCharacter->GetController(),
					this,
					UDamageType::StaticClass()
				);

				UE_LOG(LogTemp, Log, TEXT("[Shotgun] Pellet hit: %s for %.1f damage"),
					*HitActor->GetName(), PelletDamage);
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
		}
	}

	// 총 데미지 로그
	if (HasAuthority() && HitActors.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Shotgun] Total targets hit: %d"), HitActors.Num());
	}
}