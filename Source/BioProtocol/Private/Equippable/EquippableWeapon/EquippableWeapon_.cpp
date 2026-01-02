#include "Equippable/EquippableWeapon/EquippableWeapon_.h"
#include "BioProtocol/Public/Character/StaffCharacter.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AEquippableWeapon_::AEquippableWeapon_()
{
    // 기본 무기 스탯
    Damage = 10.0f;
    Range = 100.0f;
    FireRate = 1.0f;
    bIsMeleeWeapon = false;

    // 탄약
    CurrentAmmo = 0;
    MaxAmmo = 10;
    bInfiniteAmmo = false;

    // 상태
    bIsReloading = false;
    LastAttackTime = 0.0f;

    bReplicates = true;
}

void AEquippableWeapon_::BeginPlay()
{
    Super::BeginPlay();

    // 서버에서 초기 탄약 설정
    if (HasAuthority())
    {
        if (ItemReference)
        {
            CurrentAmmo = ItemReference->CurrentAmmo;
            MaxAmmo = ItemReference->MaxAmmo;
        }
        else
        {
            CurrentAmmo = MaxAmmo;
        }
    }
}

void AEquippableWeapon_::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEquippableWeapon_, CurrentAmmo);
    DOREPLIFETIME(AEquippableWeapon_, bIsReloading);
    DOREPLIFETIME(AEquippableWeapon_, LastAttackTime);
}

//==========================================
// USE OVERRIDE
//==========================================

void AEquippableWeapon_::Use()
{
    // 기본 아이템 Use 조건(장착 + OwningCharacter) 먼저 확인
    if (!Super::CanUse())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Weapon] Cannot use (not equipped or no owner): %s"), *GetName());
        return;
    }

    // 발사 가능 여부 체크
    if (!CanAttack())
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Weapon] CanAttack() == false: %s"), *GetName());
        return;
    }

    // 서버/클라 분기
    if (!HasAuthority())
    {
        ServerAttack();
        return;
    }

    // 서버에서 실제 공격 처리
    Attack();
}

void AEquippableWeapon_::StopUsing()
{
    Super::StopUsing();
}

bool AEquippableWeapon_::CanUse() const
{
    // 무기는 "사용 = 공격"이라, CanAttack 기준을 그대로 사용
    return CanAttack();
}

//==========================================
// ATTACK
//==========================================

void AEquippableWeapon_::Attack()
{
    // 여기까지 들어왔다는 것은:
    // - 서버(HasAuthority == true)
    // - CanAttack() == true
    // 조건을 만족한 상태라는 가정

    // 탄약 소모 (무한 탄약이 아니면)
    if (!bInfiniteAmmo)
    {
        CurrentAmmo--;
        OnAmmoChanged.Broadcast(CurrentAmmo);

        // ItemReference 동기화
        if (ItemReference)
        {
            ItemReference->CurrentAmmo = CurrentAmmo;
        }
    }

    // 실제 공격 처리 (자식 클래스에서 구현)
    ProcessAttack();

    // 이펙트 재생 (모든 클라이언트) - 서버 포함
    MulticastPlayAttackEffects();

    // 발사 속도 제한 갱신
    if (UWorld* World = GetWorld())
    {
        LastAttackTime = World->GetTimeSeconds();
    }

    UE_LOG(LogTemp, Log, TEXT("[Weapon] Attack - Ammo: %d/%d"), CurrentAmmo, MaxAmmo);
}

bool AEquippableWeapon_::CanAttack() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // 장착되어 있고, 재장전 중이 아니고, 탄약이 있어야 함
    if (!bIsEquipped || bIsReloading)
    {
        return false;
    }

    // 무한 탄약이면 탄약 체크 스킵
    if (!bInfiniteAmmo)
    {
        if (CurrentAmmo <= 0)
        {
            return false;
        }
    }

    // 발사 속도 제한 확인 (초당 FireRate 발)
    const float TimeSinceLastAttack = World->GetTimeSeconds() - LastAttackTime;
    const float MinTimeBetweenAttacks = (FireRate > 0.f) ? 1.0f / FireRate : 0.f;

    return TimeSinceLastAttack >= MinTimeBetweenAttacks;
}

void AEquippableWeapon_::ProcessAttack()
{
    // 자식 클래스에서 오버라이드
    // 예: LineTrace (총기), BoxTrace (근접)
    UE_LOG(LogTemp, Warning, TEXT("[Weapon] ProcessAttack() not implemented in child class: %s"), *GetName());
}

//==========================================
// RELOAD
//==========================================

void AEquippableWeapon_::Reload()
{
    if (!CanReload())
    {
        return;
    }

    // 서버에 재장전 요청
    if (!HasAuthority())
    {
        ServerReload();
        return;
    }

    bIsReloading = true;
    OnReloadStarted.Broadcast();

    // 2초 후 재장전 완료
    GetWorldTimerManager().SetTimer(
        ReloadTimerHandle,
        this,
        &AEquippableWeapon_::CompleteReload,
        2.0f,
        false
    );

    UE_LOG(LogTemp, Log, TEXT("[Weapon] Reload started: %s"), *GetName());
}

bool AEquippableWeapon_::CanReload() const
{
    // 근접 무기나 무한 탄약이면 재장전 불필요
    if (bIsMeleeWeapon || bInfiniteAmmo)
    {
        return false;
    }

    // 장착 안 되어 있거나 소유자 없으면 불가
    if (!bIsEquipped || !OwningCharacter)
    {
        return false;
    }

    // 이미 재장전 중이거나 탄창이 가득 차있으면 불가
    if (bIsReloading || CurrentAmmo >= MaxAmmo)
    {
        return false;
    }

    return true;
}

void AEquippableWeapon_::CompleteReload()
{
    if (!HasAuthority())
    {
        return;
    }

    // 탄약 보급
    CurrentAmmo = MaxAmmo;
    bIsReloading = false;

    OnAmmoChanged.Broadcast(CurrentAmmo);
    OnReloadCompleted.Broadcast();

    // ItemReference 동기화
    if (ItemReference)
    {
        ItemReference->CurrentAmmo = CurrentAmmo;
    }

    UE_LOG(LogTemp, Log, TEXT("[Weapon] Reload completed - Ammo: %d/%d (%s)"),
        CurrentAmmo, MaxAmmo, *GetName());
}

//==========================================
// AMMO MANAGEMENT
//==========================================

void AEquippableWeapon_::AddAmmo(int32 Amount)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bIsMeleeWeapon || bInfiniteAmmo)
    {
        return;
    }

    CurrentAmmo = FMath::Clamp(CurrentAmmo + Amount, 0, MaxAmmo);
    OnAmmoChanged.Broadcast(CurrentAmmo);

    // ItemReference 동기화
    if (ItemReference)
    {
        ItemReference->CurrentAmmo = CurrentAmmo;
    }

    UE_LOG(LogTemp, Log, TEXT("[Weapon] Ammo added: %d (Total: %d/%d) - %s"),
        Amount, CurrentAmmo, MaxAmmo, *GetName());
}

void AEquippableWeapon_::RefillAmmo()
{
    if (!HasAuthority())
    {
        return;
    }

    CurrentAmmo = MaxAmmo;
    OnAmmoChanged.Broadcast(CurrentAmmo);

    // ItemReference 동기화
    if (ItemReference)
    {
        ItemReference->CurrentAmmo = CurrentAmmo;
    }

    UE_LOG(LogTemp, Log, TEXT("[Weapon] Ammo refilled: %d/%d - %s"),
        CurrentAmmo, MaxAmmo, *GetName());
}

float AEquippableWeapon_::GetAmmoPercent() const
{
    if (MaxAmmo <= 0 || bInfiniteAmmo)
    {
        return 1.0f;
    }

    return static_cast<float>(CurrentAmmo) / static_cast<float>(MaxAmmo);
}

//==========================================
// EFFECTS
//==========================================

void AEquippableWeapon_::PlayAttackEffects()
{
    if (!AttackParticle)
    {
        return;
    }

    if (ItemMesh)
    {
        UGameplayStatics::SpawnEmitterAttached(
            AttackParticle,
            ItemMesh,
            FName("Muzzle"),  // 총구 소켓
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }
    else
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            AttackParticle,
            GetActorLocation()
        );
    }
}

void AEquippableWeapon_::ProcessHit(const FHitResult& HitResult)
{
    // 자식 클래스에서 구현 (데미지 외 추가 이펙트/사운드 등)
}

//==========================================
// REPLICATION
//==========================================

void AEquippableWeapon_::OnRep_CurrentAmmo()
{
    OnAmmoChanged.Broadcast(CurrentAmmo);

    UE_LOG(LogTemp, Log, TEXT("[Weapon Client] Ammo updated: %d/%d - %s"),
        CurrentAmmo, MaxAmmo, *GetName());
}

void AEquippableWeapon_::ServerAttack_Implementation()
{
    // 서버에서만 Attack 실행
    if (CanAttack())
    {
        Attack();
    }
}

void AEquippableWeapon_::ServerReload_Implementation()
{
    Reload();
}

void AEquippableWeapon_::MulticastPlayAttackEffects_Implementation()
{
    // 서버/클라 모두 동일 이펙트 재생
    PlayAttackEffects();
}
