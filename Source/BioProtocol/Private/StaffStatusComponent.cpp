// Fill out your copyright notice in the Description page of Project Settings.


#include "StaffStatusComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <StaffCharacter.h>
#include <MyPlayerController.h>
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
	DOREPLIFETIME(UStaffStatusComponent, CurrentHP);
	DOREPLIFETIME(UStaffStatusComponent, CurrentMoveSpeed);
	DOREPLIFETIME(UStaffStatusComponent, CurrentAttack);
	DOREPLIFETIME(UStaffStatusComponent, LifeState);
	DOREPLIFETIME(UStaffStatusComponent, CurrentStamina);
	DOREPLIFETIME(UStaffStatusComponent, bIsRunable);
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

void UStaffStatusComponent::OnRep_LifeState()
{
	if (LifeState == ECharacterLifeState::Dead)
	{
		if (AStaffCharacter* OwnerChar = Cast<AStaffCharacter>(GetOwner()))
		{
			OwnerChar->OnDeath();
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Dead"));
		}
	}
}

void UStaffStatusComponent::OnRep_CurrentHP()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
		FString::Printf(TEXT("%s HP: %f (RepNotified)"), *GetOwner()->GetName(), CurrentHP));
}

void UStaffStatusComponent::OnRep_CurrentStamina()
{
	//UE_LOG(LogTemp, Warning, TEXT("CLIENT Stamina Updated: %f"), CurrentStamina);

}

void UStaffStatusComponent::ApplyDamage(float Damage)
{
	if (!GetOwner()->HasAuthority()) return;
	if (LifeState == ECharacterLifeState::Dead) return;

	const float FinalDamage = FMath::Max(Damage - Defense, 1.f);
	CurrentHP -= FinalDamage;

	UE_LOG(LogTemp, Warning, TEXT("%s - CurrentHP: %f (Server)"), *GetOwner()->GetName(), CurrentHP);

	if (CurrentHP <= 0.f)
	{
		CurrentHP = 0.f;
		LifeState = ECharacterLifeState::Dead;

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			OwnerCharacter->GetCharacterMovement()->DisableMovement();

			if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
			{
				/*	if (AMyPlayerController* MyPC = Cast<AMyPlayerController>(PC))
					{
						MyPC->ClientStartSpectate();
					}*/
				AMyGameModeBase* GM = Cast<AMyGameModeBase>(GetWorld()->GetAuthGameMode());
				if (GM)
				{
					GM->OnPlayerKilled(PC);
				}
			}
		}


	}
}

