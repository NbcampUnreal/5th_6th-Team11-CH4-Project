#include "Equippable/EquippableWeapon/EquippableWeapon_Pistol.h"
#include "BioProtocol/Public/Character/StaffCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AEquippableWeapon_Pistol::AEquippableWeapon_Pistol()
{
    // 권총 스탯
    Damage = 25.0f;
    Range = 5000.0f;   // 50m
    FireRate = 3.0f;      // 초당 3발
    bIsMeleeWeapon = false;
    bInfiniteAmmo = false;
    MaxAmmo = 12;
    CurrentAmmo = MaxAmmo;

    MuzzleSocketName = FName("Muzzle");
    ImpactParticle = nullptr;
}

void AEquippableWeapon_Pistol::ProcessAttack()
{
    // AEquippableWeapon_::Attack()에서 서버에서 호출된다는 가정
    if (!OwningCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pistol] No OwningCharacter on ProcessAttack: %s"), *GetName());
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 카메라 기준으로 조준
    UCameraComponent* Camera = OwningCharacter->FindComponentByClass<UCameraComponent>();
    if (!Camera)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pistol] No CameraComponent on owner: %s"), *OwningCharacter->GetName());
        return;
    }

    // LineTrace 시작/끝 위치
    // 카메라 위치에서 쏴도 되지만, 지금은 "총구 위치에서 카메라 방향"을 사용
    const FVector Start = OwningCharacter->GetMesh()->GetSocketLocation(MuzzleSocketName);
    const FVector Forward = Camera->GetForwardVector();
    const FVector End = Start + (Forward * Range);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PistolTrace), false, this);
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(OwningCharacter);
    QueryParams.bTraceComplex = true;

    const bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_Visibility,
        QueryParams
    );

    // 디버그 라인
    DrawDebugLine(
        World,
        Start,
        bHit ? HitResult.Location : End,
        bHit ? FColor::Red : FColor::Green,
        false,
        1.0f,
        0,
        1.0f
    );

    if (!bHit)
    {
        return;
    }

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
            World,
            ImpactParticle,
            HitResult.Location,
            HitResult.Normal.Rotation()
        );
    }

    // 베이스에서 오버라이드할 수 있는 후처리 훅
    ProcessHit(HitResult);
}
