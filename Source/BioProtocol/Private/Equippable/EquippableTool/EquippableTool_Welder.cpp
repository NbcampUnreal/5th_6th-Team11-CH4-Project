// EquippableTool_Welder.cpp
#include "BioProtocol/Public/Equippable/EquippableTool/EquippableTool_Welder.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

class AEquippableItem;

AEquippableTool_Welder::AEquippableTool_Welder()
{
	PrimaryActorTick.bCanEverTick = true;

	// 기본 내구도 설정
	MaxDurability = 100.0f;
	CurrentDurability = MaxDurability;
	DurabilityConsumeRate = 5.0f;  // 초당 5 소모
	LowDurabilityThreshold = 20.0f; // 20% 이하면 경고

	// VFX/Audio 컴포넌트 초기화
	ActiveWeldingParticle = nullptr;
	ActiveWeldingSound = nullptr;
	WeldingParticle = nullptr;
	WeldingSound = nullptr;
}

void AEquippableTool_Welder::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 내구도 초기화
	if (HasAuthority())
	{
		// ItemReference에서 내구도 정보 가져오기
		if (ItemReference && ItemReference->NumericData.MaxDurability > 0.0f)
		{
			MaxDurability = ItemReference->NumericData.MaxDurability;
			CurrentDurability = ItemReference->NumericData.Durability;
		}
	}
}

void AEquippableTool_Welder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 사용 중이면 내구도 소모
	if (bIsInUse && HasAuthority())
	{
		ProcessDurabilityConsumption(DeltaTime);
	}
}

void AEquippableTool_Welder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEquippableTool_Welder, CurrentDurability);
}

//==========================================
// USE FUNCTIONS
//==========================================

void AEquippableTool_Welder::Use()
{
	if (!CanUse())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Welder] Cannot use - no durability or not equipped"));
		return;
	}

	Super::Use();

	// 이펙트 시작
	StartWeldingEffects();

	UE_LOG(LogTemp, Log, TEXT("[Welder] Started using - Durability: %.1f/%.1f"),
		CurrentDurability, MaxDurability);
}

void AEquippableTool_Welder::StopUsing()
{
	Super::StopUsing();

	// 이펙트 중지
	StopWeldingEffects();

	UE_LOG(LogTemp, Log, TEXT("[Welder] Stopped using - Durability: %.1f/%.1f"),
		CurrentDurability, MaxDurability);
}

bool AEquippableTool_Welder::CanUse() const
{
	// 장착되어 있고, 내구도가 남아있어야 사용 가능
	return Super::CanUse() && CurrentDurability > 0.0f;
}

//==========================================
// DURABILITY SYSTEM
//==========================================

void AEquippableTool_Welder::ProcessDurabilityConsumption(float DeltaTime)
{
	if (CurrentDurability <= 0.0f)
	{
		// 내구도 소진 → 자동으로 사용 중지
		StopUsing();
		return;
	}

	// 내구도 감소
	float ConsumeAmount = DurabilityConsumeRate * DeltaTime;
	ConsumeDurability(ConsumeAmount);
}

void AEquippableTool_Welder::ConsumeDurability(float Amount)
{
	if (!HasAuthority())
	{
		ServerConsumeDurability(Amount);
		return;
	}

	if (Amount <= 0.0f)
	{
		return;
	}

	// 내구도 감소
	CurrentDurability = FMath::Max(CurrentDurability - Amount, 0.0f);

	// 델리게이트 브로드캐스트
	OnDurabilityChanged.Broadcast(CurrentDurability);

	// ItemReference 동기화
	if (ItemReference)
	{
		ItemReference->NumericData.Durability = CurrentDurability;
	}

	// 내구도 소진 시 자동 중지
	if (CurrentDurability <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Welder] Durability depleted!"));
		StopUsing();
	}
	// 낮은 내구도 경고
	else if (GetDurabilityPercent() <= (LowDurabilityThreshold / 100.0f))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Welder] Low durability warning: %.1f%%"),
			GetDurabilityPercent() * 100.0f);
	}
}

void AEquippableTool_Welder::RestoreDurability(float Amount)
{
	if (!HasAuthority())
	{
		ServerRestoreDurability(Amount);
		return;
	}

	if (Amount <= 0.0f)
	{
		return;
	}

	// 내구도 회복
	CurrentDurability = FMath::Min(CurrentDurability + Amount, MaxDurability);

	// 델리게이트 브로드캐스트
	OnDurabilityChanged.Broadcast(CurrentDurability);

	// ItemReference 동기화
	if (ItemReference)
	{
		ItemReference->NumericData.Durability = CurrentDurability;
	}

	UE_LOG(LogTemp, Log, TEXT("[Welder] Durability restored: %.1f/%.1f (%.1f%%)"),
		CurrentDurability, MaxDurability, GetDurabilityPercent() * 100.0f);
}

void AEquippableTool_Welder::FullyRecharge()
{
	RestoreDurability(MaxDurability);
}

//==========================================
// VISUAL & AUDIO EFFECTS
//==========================================

void AEquippableTool_Welder::StartWeldingEffects()
{
	// 파티클 이펙트 생성
	if (WeldingParticle && ItemMesh && !ActiveWeldingParticle)
	{
		ActiveWeldingParticle = UGameplayStatics::SpawnEmitterAttached(
			WeldingParticle,
			ItemMesh,
			NAME_None,  // 소켓 이름 (없으면 루트)
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true  // 자동 파괴
		);

		if (ActiveWeldingParticle)
		{
			UE_LOG(LogTemp, Log, TEXT("[Welder] Welding particle started"));
		}
	}

	// 사운드 재생
	if (WeldingSound && ItemMesh && !ActiveWeldingSound)
	{
		ActiveWeldingSound = UGameplayStatics::SpawnSoundAttached(
			WeldingSound,
			ItemMesh,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget,
			false,  // 자동 파괴 안함
			1.0f,   // 볼륨
			1.0f,   // 피치
			0.0f,   // 시작 시간
			nullptr,
			nullptr,
			true    // 자동 활성화
		);

		if (ActiveWeldingSound)
		{
			UE_LOG(LogTemp, Log, TEXT("[Welder] Welding sound started"));
		}
	}
}

void AEquippableTool_Welder::StopWeldingEffects()
{
	// 파티클 중지
	if (ActiveWeldingParticle)
	{
		ActiveWeldingParticle->DeactivateSystem();
		ActiveWeldingParticle = nullptr;
		UE_LOG(LogTemp, Log, TEXT("[Welder] Welding particle stopped"));
	}

	// 사운드 중지
	if (ActiveWeldingSound)
	{
		ActiveWeldingSound->Stop();
		ActiveWeldingSound = nullptr;
		UE_LOG(LogTemp, Log, TEXT("[Welder] Welding sound stopped"));
	}
}

//==========================================
// REPLICATION
//==========================================

void AEquippableTool_Welder::OnRep_CurrentDurability()
{
	// 클라이언트에서 내구도 변경 시 UI 업데이트
	OnDurabilityChanged.Broadcast(CurrentDurability);

	// ItemReference 동기화
	if (ItemReference)
	{
		ItemReference->NumericData.Durability = CurrentDurability;
	}

	UE_LOG(LogTemp, Log, TEXT("[Welder Client] Durability updated: %.1f/%.1f (%.1f%%)"),
		CurrentDurability, MaxDurability, GetDurabilityPercent() * 100.0f);

	// 내구도 소진 시 이펙트 중지
	if (CurrentDurability <= 0.0f && bIsInUse)
	{
		StopWeldingEffects();
	}
}

//==========================================
// SERVER RPC
//==========================================

void AEquippableTool_Welder::ServerConsumeDurability_Implementation(float Amount)
{
	ConsumeDurability(Amount);
}

bool AEquippableTool_Welder::ServerConsumeDurability_Validate(float Amount)
{
	// 유효성 검사: 음수 값이나 비정상적으로 큰 값 방지
	return Amount >= 0.0f && Amount <= MaxDurability;
}

void AEquippableTool_Welder::ServerRestoreDurability_Implementation(float Amount)
{
	RestoreDurability(Amount);
}

bool AEquippableTool_Welder::ServerRestoreDurability_Validate(float Amount)
{
	// 유효성 검사: 음수 값이나 비정상적으로 큰 값 방지
	return Amount >= 0.0f && Amount <= MaxDurability;
}