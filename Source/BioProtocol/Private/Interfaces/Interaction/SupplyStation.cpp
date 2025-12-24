// Fill out your copyright notice in the Description page of Project Settings.


#include "Interfaces/Interaction/SupplyStation.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "BioProtocol/Public/Equippable/EquippableTool/EquippableTool_Welder.h"
#include "BioProtocol/Public/Interfaces/InteractionInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ASupplyStation::ASupplyStation()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	StationMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	StationMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SetRootComponent(StationMesh);

	RechargeTime = 5.0f;
	RechargeRatePerSecond = 20.0f;
	bIsCharging = false;
	ChargingPlayer = nullptr;

	ChargingParticle = nullptr;
	ChargingSound = nullptr;
	ActiveChargingParticle = nullptr;
	ActiveChargingSound = nullptr;

	InstanceInteractableData.InteractableType = EInteractableType::SupplyStation;
	InstanceInteractableData.Name = FText::FromString("Supply Station");
	InstanceInteractableData.Action = FText::FromString("Recharge Welder");
	InstanceInteractableData.InteractionDuration = RechargeTime;
	InstanceInteractableData.bCanInteract = true;
}

void ASupplyStation::BeginPlay()
{
	Super::BeginPlay();

	InteractableData = InstanceInteractableData;
}

void ASupplyStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASupplyStation, bIsCharging);
	DOREPLIFETIME(ASupplyStation, ChargingPlayer);
}

//==========================================
// INTERACTION INTERFACE
//==========================================

void ASupplyStation::BeginFocus_Implementation()
{
	if (StationMesh)
	{
		StationMesh->SetRenderCustomDepth(true);
	}
}

void ASupplyStation::EndFocus_Implementation()
{
	if (StationMesh)
	{
		StationMesh->SetRenderCustomDepth(false);
	}
}

void ASupplyStation::BeginInteract_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("[SupplyStation] BeginInteract"));
}

void ASupplyStation::EndInteract_Implementation()
{
	if (bIsCharging)
	{
		CancelCharging();
	}
}

void ASupplyStation::Interact_Implementation(ADXPlayerCharacter* PlayerCharacter)
{
	if (!PlayerCharacter || !HasAuthority())
	{
		return;
	}

	StartCharging(PlayerCharacter);
}

FInteractableData ASupplyStation::GetInteractableData_Implementation() const
{
	return InstanceInteractableData;
}

bool ASupplyStation::CanInteract_Implementation(ADXPlayerCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter || bIsCharging)
	{
		return false;
	}

	// 플레이어가 용접기를 들고 있는지 확인
	AEquippableItem* CurrentItem = PlayerCharacter->GetCurrentEquippedItem();
	AEquippableTool_Welder* Welder = Cast<AEquippableTool_Welder>(CurrentItem);

	if (!Welder)
	{
		return false;
	}

	// 이미 완충 상태면 충전 불가
	if (Welder->GetDurabilityPercent() >= 1.0f)
	{
		return false;
	}

	return true;
}

//==========================================
// CHARGING FUNCTIONS
//==========================================

void ASupplyStation::StartCharging(ADXPlayerCharacter* Player)
{
	if (!HasAuthority())
	{
		ServerStartCharging(Player);
		return;
	}

	if (!Player || bIsCharging)
	{
		return;
	}

	// 용접기 확인
	AEquippableItem* CurrentItem = Player->GetCurrentEquippedItem();
	AEquippableTool_Welder* Welder = Cast<AEquippableTool_Welder>(CurrentItem);

	if (!Welder)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SupplyStation] Player doesn't have a welder equipped"));
		return;
	}

	bIsCharging = true;
	ChargingPlayer = Player;

	StartChargingEffects();

	// 즉시 완충 모드 (RechargeRatePerSecond가 0이면)
	if (RechargeRatePerSecond <= 0.0f)
	{
		Welder->FullyRecharge();
		CompleteCharging();
		return;
	}

	// 점진적 충전 모드
	GetWorldTimerManager().SetTimer(
		ChargingTimerHandle,
		this,
		&ASupplyStation::ProcessCharging,
		0.1f,
		true
	);

	UE_LOG(LogTemp, Log, TEXT("[SupplyStation] Started charging for %s"), *Player->GetName());
}

void ASupplyStation::ProcessCharging()
{
	if (!ChargingPlayer || !bIsCharging)
	{
		CancelCharging();
		return;
	}

	// 용접기 가져오기
	AEquippableItem* CurrentItem = ChargingPlayer->GetCurrentEquippedItem();
	AEquippableTool_Welder* Welder = Cast<AEquippableTool_Welder>(CurrentItem);

	if (!Welder)
	{
		CancelCharging();
		return;
	}

	// 충전 진행
	float ChargeAmount = RechargeRatePerSecond * 0.1f;
	Welder->RestoreDurability(ChargeAmount);

	// 완충 확인
	if (Welder->GetDurabilityPercent() >= 1.0f)
	{
		CompleteCharging();
	}
}

void ASupplyStation::CompleteCharging()
{
	if (!HasAuthority())
	{
		ServerCompleteCharging();
		return;
	}

	GetWorldTimerManager().ClearTimer(ChargingTimerHandle);
	StopChargingEffects();

	bIsCharging = false;
	ChargingPlayer = nullptr;

	UE_LOG(LogTemp, Log, TEXT("[SupplyStation] Charging completed"));
}

void ASupplyStation::CancelCharging()
{
	if (!HasAuthority())
	{
		ServerCancelCharging();
		return;
	}

	GetWorldTimerManager().ClearTimer(ChargingTimerHandle);
	StopChargingEffects();

	bIsCharging = false;
	ChargingPlayer = nullptr;

	UE_LOG(LogTemp, Log, TEXT("[SupplyStation] Charging cancelled"));
}

//==========================================
// EFFECTS
//==========================================

void ASupplyStation::StartChargingEffects()
{
	if (ChargingParticle && StationMesh && !ActiveChargingParticle)
	{
		ActiveChargingParticle = UGameplayStatics::SpawnEmitterAttached(
			ChargingParticle,
			StationMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}

	if (ChargingSound && StationMesh && !ActiveChargingSound)
	{
		ActiveChargingSound = UGameplayStatics::SpawnSoundAttached(
			ChargingSound,
			StationMesh,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget,
			false,
			1.0f,
			1.0f,
			0.0f,
			nullptr,
			nullptr,
			true
		);
	}
}

void ASupplyStation::StopChargingEffects()
{
	if (ActiveChargingParticle)
	{
		ActiveChargingParticle->DeactivateSystem();
		ActiveChargingParticle = nullptr;
	}

	if (ActiveChargingSound)
	{
		ActiveChargingSound->Stop();
		ActiveChargingSound = nullptr;
	}
}

//==========================================
// SERVER RPC
//==========================================

void ASupplyStation::ServerStartCharging_Implementation(ADXPlayerCharacter* Player)
{
	StartCharging(Player);
}

void ASupplyStation::ServerCompleteCharging_Implementation()
{
	CompleteCharging();
}

void ASupplyStation::ServerCancelCharging_Implementation()
{
	CancelCharging();
}

