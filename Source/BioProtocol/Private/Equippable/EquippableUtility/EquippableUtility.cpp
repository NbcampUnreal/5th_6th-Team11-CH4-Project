// EquippableUtility.cpp
#include "BioProtocol/Public/Equippable/EquippableUtility/EquippableUtility.h"
#include "BioProtocol/Public/Character/StaffCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AEquippableUtility::AEquippableUtility()
{
	bConsumable = true;
	UsesRemaining = 1;
	MaxUses = 1;
	Cooldown = 0.0f;
	LastUseTime = 0.0f;
	bIsOnCooldown = false;

	UseParticle = nullptr;
	UseSound = nullptr;
}

void AEquippableUtility::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		UsesRemaining = MaxUses;
	}
}

void AEquippableUtility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEquippableUtility, UsesRemaining);
}

//==========================================
// USE OVERRIDE
//==========================================

void AEquippableUtility::Use()
{
	Super::Use();
	UseUtility();
}

bool AEquippableUtility::CanUse() const
{
	return Super::CanUse() && CanUseUtility();
}

//==========================================
// UTILITY FUNCTIONS
//==========================================

void AEquippableUtility::UseUtility()
{
	if (!CanUseUtility())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Utility] Cannot use - cooldown or no uses left"));
		return;
	}

	// 서버에 사용 요청
	if (!HasAuthority())
	{
		return;
	}

	// 실제 효과 처리
	ProcessUtilityEffect();

	// 사용 횟수 감소
	ConsumeUse();

	// 쿨다운 시작
	LastUseTime = GetWorld()->GetTimeSeconds();
	bIsOnCooldown = true;

	// 이펙트 재생
	MulticastPlayUseEffects();

	UE_LOG(LogTemp, Log, TEXT("[Utility] Used - Remaining: %d/%d"), UsesRemaining, MaxUses);

	// 쿨다운 타이머
	if (Cooldown > 0.0f)
	{
		FTimerHandle CooldownTimer;
		GetWorldTimerManager().SetTimer(
			CooldownTimer,
			[this]() { bIsOnCooldown = false; },
			Cooldown,
			false
		);
	}
	else
	{
		bIsOnCooldown = false;
	}
}

bool AEquippableUtility::CanUseUtility() const
{
	// 장착되어 있지 않으면 불가
	if (!bIsEquipped)
	{
		return false;
	}

	// 쿨다운 중이면 불가
	if (IsInCooldown())
	{
		return false;
	}

	// 사용 횟수 체크 (0이면 무제한)
	if (MaxUses > 0 && UsesRemaining <= 0)
	{
		return false;
	}

	return true;
}

void AEquippableUtility::ProcessUtilityEffect()
{
	// 자식 클래스에서 오버라이드
	UE_LOG(LogTemp, Warning, TEXT("[Utility] ProcessUtilityEffect() not implemented"));
}

//==========================================
// INTERNAL
//==========================================

void AEquippableUtility::ConsumeUse()
{
	if (!HasAuthority())
	{
		return;
	}

	if (MaxUses <= 0)
	{
		return; // 무제한 사용
	}

	UsesRemaining = FMath::Max(UsesRemaining - 1, 0);
	OnUsesChanged.Broadcast(UsesRemaining);

	// 사용 횟수 소진
	if (UsesRemaining <= 0)
	{
		OnUtilityDepleted.Broadcast();

		// 소모품이면 아이템 파괴
		if (bConsumable)
		{
			UE_LOG(LogTemp, Log, TEXT("[Utility] Consumable depleted, destroying..."));

			// 1초 후 파괴 (이펙트가 재생될 시간 확보)
			FTimerHandle DestroyTimer;
			GetWorldTimerManager().SetTimer(
				DestroyTimer,
				[this]() { Destroy(); },
				1.0f,
				false
			);
		}
	}
}

bool AEquippableUtility::IsInCooldown() const
{
	if (Cooldown <= 0.0f)
	{
		return false;
	}

	float TimeSinceLastUse = GetWorld()->GetTimeSeconds() - LastUseTime;
	return TimeSinceLastUse < Cooldown;
}

void AEquippableUtility::PlayUseEffects()
{
	// 파티클
	if (UseParticle && ItemMesh)
	{
		UGameplayStatics::SpawnEmitterAttached(
			UseParticle,
			ItemMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}

	// 사운드
	if (UseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, UseSound, GetActorLocation());
	}
}

//==========================================
// REPLICATION
//==========================================

void AEquippableUtility::OnRep_UsesRemaining()
{
	OnUsesChanged.Broadcast(UsesRemaining);

	if (UsesRemaining <= 0)
	{
		OnUtilityDepleted.Broadcast();
	}
}


void AEquippableUtility::MulticastPlayUseEffects_Implementation()
{
	if (!HasAuthority())
	{
		PlayUseEffects();
	}
}