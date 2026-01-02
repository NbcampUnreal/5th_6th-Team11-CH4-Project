#include "Equippable/EquippableWeapon/EquippableWeapon_Shotgun.h"
#include "BioProtocol/Public/Character/StaffCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

AEquippableWeapon_Shotgun::AEquippableWeapon_Shotgun()
{
    // 샷건 스탯
    Damage = 80.0f;    // 전체 한 발 기준 총 데미지
    Range = 1000.0f;  // 10m
    FireRate = 1.0f;     // 초당 1발
    bIsMeleeWeapon = false;
    bInfiniteAmmo = false;
    MaxAmmo = 6;
    CurrentAmmo = MaxAmmo;

    // 산탄 설정
    PelletCount = 8;         // 8발의 산탄
    SpreadAngle = 5.0f;      // 5도 확산

    ImpactParticle = nullptr;
}

void AEquippableWeapon_Shotgun::ProcessAttack()
{
    // AEquippableWeapon_::Attack()에서 서버에서만 호출된다는 가정
    if (!OwningCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Shotgun] No OwningCharacter on ProcessAttack: %s"), *GetName());
        return;
    }

    // 1. 카메라 가져오기 (1인칭 기준)
    UCameraComponent* Camera = OwningCharacter->FindComponentByClass<UCameraComponent>();
    if (!Camera)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Shotgun] No CameraComponent on owner: %s"), *OwningCharacter->GetName());
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector CameraLocation = Camera->GetComponentLocation();
    const FVector CameraForward = Camera->GetForwardVector();

    // 중복 타겟에 중복 데미지 들어가는 것 방지용
    TMap<AActor*, float> AccumulatedDamagePerActor;

    for (int32 i = 0; i < PelletCount; ++i)
    {
        // 2. 랜덤 확산 방향 계산
        const FVector PelletDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(
            CameraForward,
            SpreadAngle
        );

        const FVector TraceStart = CameraLocation;
        const FVector TraceEnd = TraceStart + PelletDir * Range;

        // 3. 라인트레이스
        FHitResult HitResult;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShotgunTrace), false, this);
        QueryParams.AddIgnoredActor(this);
        QueryParams.AddIgnoredActor(OwningCharacter);
        QueryParams.bTraceComplex = true;

        const bool bHit = World->LineTraceSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            ECC_Visibility,
            QueryParams
        );

        // 디버그 라인
        DrawDebugLine(
            World,
            TraceStart,
            bHit ? HitResult.Location : TraceEnd,
            bHit ? FColor::Red : FColor::Orange,
            false,
            0.5f,
            0,
            0.5f
        );

        if (!bHit)
        {
            continue;
        }

        AActor* HitActor = HitResult.GetActor();
        if (!HitActor)
        {
            continue;
        }

        // 4. 데미지 누적 (PelletDamage 합산 방식)
        const float PelletDamage = Damage / static_cast<float>(PelletCount);

        float& AccDamage = AccumulatedDamagePerActor.FindOrAdd(HitActor);
        AccDamage += PelletDamage;

        // 5. 히트 이펙트(파티클)
        if (ImpactParticle)
        {
            UGameplayStatics::SpawnEmitterAtLocation(
                World,
                ImpactParticle,
                HitResult.Location,
                HitResult.Normal.Rotation()
            );
        }
    }

    // 6. 서버에서 누적 데미지 적용
    if (HasAuthority())
    {
        for (const TPair<AActor*, float>& Pair : AccumulatedDamagePerActor)
        {
            AActor* DamagedActor = Pair.Key;
            const float TotalDamage = Pair.Value;

            UGameplayStatics::ApplyPointDamage(
                DamagedActor,
                TotalDamage,
                (DamagedActor->GetActorLocation() - OwningCharacter->GetActorLocation()).GetSafeNormal(),
                FHitResult(), // 정확한 Hit 정보가 필요하면 위에서 저장한 마지막 HitResult를 같이 넘겨도 됨
                OwningCharacter->GetController(),
                this,
                UDamageType::StaticClass()
            );

            UE_LOG(LogTemp, Log, TEXT("[Shotgun] Hit %s for %.1f total damage"),
                *DamagedActor->GetName(), TotalDamage);
        }

        if (AccumulatedDamagePerActor.Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("[Shotgun] Total targets hit: %d"), AccumulatedDamagePerActor.Num());
        }
    }
}
