// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/StaffStatusComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Character/StaffCharacter.h>
#include <Character/MyPlayerController.h>
#include "Game/BioGameMode.h"
#include "MyGameModeBase.h"

// Sets default values for this component's properties
UStaffStatusComponent::UStaffStatusComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	// ...
}

void UStaffStatusComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps); 

	DOREPLIFETIME(UStaffStatusComponent, CurrentHP);
	DOREPLIFETIME(UStaffStatusComponent, CurrentMoveSpeed);
	DOREPLIFETIME(UStaffStatusComponent, CurrentAttack);
	DOREPLIFETIME(UStaffStatusComponent, CurrentStamina);
	DOREPLIFETIME(UStaffStatusComponent, PlayerStatus);
	DOREPLIFETIME(UStaffStatusComponent, bIsTransformed);
	DOREPLIFETIME(UStaffStatusComponent, bIsRunable);
	DOREPLIFETIME(UStaffStatusComponent, JailTimer);
}



// Called when the game starts
void UStaffStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	//if (GetOwner()->HasAuthority())
	//{
	//	ApplyBaseStatus();
	//}

}


// Called every frame
void UStaffStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UStaffStatusComponent::ApplyBaseStatus()
{
	CurrentHP = MaxHP;
	CurrentStamina = MaxStamina;
	CurrentMoveSpeed = MoveSpeed;
	CurrentAttack = Attack;
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}
}


void UStaffStatusComponent::StartConsumeStamina()
{
	if (!GetOwner()->HasAuthority())
		return;

	if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_ConsumeStamina))
		return;

	StopRegenStamina();

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_ConsumeStamina,
		this,
		&UStaffStatusComponent::ConsumeStaminaTick,
		0.1f,   // 10Hz
		true
	);
}

void UStaffStatusComponent::StopConsumeStamina()
{
	if (!GetOwner()->HasAuthority())
		return;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ConsumeStamina);
}

void UStaffStatusComponent::StartRegenStamina()
{
	if (!GetOwner()->HasAuthority())
		return;

	StopConsumeStamina();

	if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_RegenStamina))
		return;

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_RegenStamina,
		this,
		&UStaffStatusComponent::RegenStaminaTick,
		0.1f,
		true
	);
}

void UStaffStatusComponent::StopRegenStamina()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RegenStamina);
}

void UStaffStatusComponent::ConsumeStaminaTick()
{
	float TickInterval = 0.1f;

	CurrentStamina -= StaminaReduce * TickInterval;
	CurrentStamina = FMath::Max(CurrentStamina, 0.f);
	//UE_LOG(LogTemp, Error, TEXT("%f"), CurrentStamina);

	if (CurrentStamina <= 0.f)
	{
		bIsRunable = false;
		StopConsumeStamina();
	}

	if (CurrentStamina <= 0.f)
	{
		CurrentStamina = 0.f;
		bIsRunable = false;

		StopConsumeStamina();

		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_SetRunable,
			this,
			&UStaffStatusComponent::SetRunable,
			3.f,
			false
		);
	}
}

void UStaffStatusComponent::RegenStaminaTick()
{
	constexpr float TickInterval = 0.1f;

	CurrentStamina += StaminaIncrease * TickInterval;
	CurrentStamina = FMath::Min(CurrentStamina, MaxStamina);
	//UE_LOG(LogTemp, Error, TEXT("%f"), CurrentStamina);

	if (CurrentStamina >= MaxStamina)
	{
		StopRegenStamina();
	}
}

bool UStaffStatusComponent::CanJumpByStamina() const
{
	return CurrentStamina >= JumpStamina;
}

void UStaffStatusComponent::ConsumeJumpStamina()
{
	if (!GetOwner()->HasAuthority())
		return;

	CurrentStamina -= JumpStamina;
	CurrentStamina = FMath::Max(CurrentStamina, 0.f);

	if (CurrentStamina <= 0.f)
	{
		bIsRunable = false;

		StopConsumeStamina();
	}

	if (CurrentStamina <= 0.f)
	{
		CurrentStamina = 0.f;
		bIsRunable = false;

		StopConsumeStamina();

		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_SetRunable,
			this,
			&UStaffStatusComponent::SetRunable,
			3.f,
			false
		);
	}
}

void UStaffStatusComponent::OnRep_CurrentHP()
{
	OnHPChanged.Broadcast(CurrentHP);
}

void UStaffStatusComponent::OnRep_CurrentStamina()
{
	//UE_LOG(LogTemp, Warning, TEXT("CLIENT Stamina Updated: %f"), CurrentStamina);
	OnStaminaChanged.Broadcast(CurrentStamina);
}

void UStaffStatusComponent::OnRep_PlayerStatus()
{
	OnStatusChanged.Broadcast(PlayerStatus);
}
void UStaffStatusComponent::OnRep_IsTransformed()
{
	OnTransformChanged.Broadcast(bIsTransformed);
}

void UStaffStatusComponent::ApplyDamage(float Damage,AController* DamageInstigator)
{
	if (!GetOwner()->HasAuthority()) return;
	if (PlayerStatus == EBioPlayerStatus::Dead) return;

	CurrentHP -= Damage;

	//UE_LOG(LogTemp, Warning, TEXT("%s - CurrentHP: %f (Server)"), *GetOwner()->GetName(), CurrentHP);

	OnHPChanged.Broadcast(CurrentHP);

	if (CurrentHP <= 0.f)
	{
		CurrentHP = 0.f;
		SetJailed();
		PlayerStatus = EBioPlayerStatus::Jailed;
	}
}

void UStaffStatusComponent::SetJailed()
{
	if (!GetOwner()->HasAuthority()) return;

	if (PlayerStatus != EBioPlayerStatus::Alive) return;

	PlayerStatus = EBioPlayerStatus::Jailed;
	OnRep_PlayerStatus();

	UE_LOG(LogTemp, Warning, TEXT("[StaffStatus] Player Jailed. Requesting GameMode Logic."));

	if (ABioGameMode* GM = Cast<ABioGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
		{
			GM->SendPlayerToJail(OwningPawn);
		}
	}
}

void UStaffStatusComponent::UpdateJailTime(float NewTime)
{
	if (GetOwner()->HasAuthority())
	{
		JailTimer = NewTime;
	}
}

void UStaffStatusComponent::ServerSetTransform_Implementation(bool bNewState)
{
	bIsTransformed = bNewState;

	if (bIsTransformed)
	{
		MaxHP = 200.f;
		CurrentHP = MaxHP;
	}
	else
	{
		MaxHP = 100.f;
		CurrentHP = FMath::Min(CurrentHP, MaxHP);
	}

	OnRep_IsTransformed();
}

void UStaffStatusComponent::SetDead()
{
	if (!GetOwner()->HasAuthority()) return;
	if (PlayerStatus == EBioPlayerStatus::Dead) return;

	PlayerStatus = EBioPlayerStatus::Dead;
	CurrentHP = 0.0f;
	OnRep_PlayerStatus();

	UE_LOG(LogTemp, Error, TEXT("[StaffStatus] SetDead Called by GameMode."));

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		OwnerChar->GetCharacterMovement()->DisableMovement();
	}
}

void UStaffStatusComponent::SetRevived()
{
	if (!GetOwner()->HasAuthority()) return;

	PlayerStatus = EBioPlayerStatus::Alive;
	JailTimer = 0.0f;
	CurrentHP = 20.0f;
	OnRep_PlayerStatus();

	UE_LOG(LogTemp, Log, TEXT("[StaffStatus] Player Revived via Lever!"));

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		OwnerChar->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}