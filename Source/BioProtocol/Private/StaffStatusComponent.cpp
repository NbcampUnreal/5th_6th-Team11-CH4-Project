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
}



// Called when the game starts
void UStaffStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		ApplyBaseStatus();
	}

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
	CurrentMoveSpeed = MoveSpeed;
	CurrentAttack = Attack;
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
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
	FString::Printf(TEXT("%s HP: %f (RepNotified)"),*GetOwner()->GetName(), CurrentHP));
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

