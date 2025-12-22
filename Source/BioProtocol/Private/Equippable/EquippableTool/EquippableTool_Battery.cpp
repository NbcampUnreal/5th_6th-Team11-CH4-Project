#include "Equippable/EquippableTool/EquippableTool_Battery.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

AEquippableTool_Battery::AEquippableTool_Battery()
{
	PrimaryActorTick.bCanEverTick = true;

	// 기본 이동속도 감소율 50%
	MovementSpeedModifier = 0.5f;
	OriginalMaxWalkSpeed = 0.0f;
	bIsCarrying = false;
}

void AEquippableTool_Battery::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[Battery] Battery initialized"));
}

void AEquippableTool_Battery::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 배터리 들고 있을 때 지속적으로 이동속도 체크
	if (bIsCarrying && OwningCharacter)
	{
		UCharacterMovementComponent* Movement = OwningCharacter->GetCharacterMovement();
		if (Movement)
		{
			float ExpectedSpeed = OriginalMaxWalkSpeed * MovementSpeedModifier;
			if (FMath::Abs(Movement->MaxWalkSpeed - ExpectedSpeed) > 1.0f)
			{
				// 속도가 변경되었다면 다시 적용
				ApplyMovementPenalty();
			}
		}
	}
}
void AEquippableTool_Battery::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEquippableTool_Battery, bIsCarrying);
}

void AEquippableTool_Battery::Use()
{
	Super::Use();
	UE_LOG(LogTemp, Log, TEXT("[Battery] Being carried"));
}

bool AEquippableTool_Battery::CanUse() const
{
	return Super::CanUse() && bIsCarrying;
}

void AEquippableTool_Battery::Equip()
{
	Super::Equip();

	if (!HasAuthority())
	{
		ServerApplyMovementPenalty();
		return;
	}

	ApplyMovementPenalty();
}

void AEquippableTool_Battery::Unequip()
{
	Super::Unequip();

	if (!HasAuthority())
	{
		ServerRemoveMovementPenalty();
		return;
	}

	RemoveMovementPenalty();
}

void AEquippableTool_Battery::ApplyMovementPenalty()
{
	if (!OwningCharacter) return;

	UCharacterMovementComponent* Movement = OwningCharacter->GetCharacterMovement();
	if (!Movement) return;

	if (OriginalMaxWalkSpeed == 0.0f)
	{
		OriginalMaxWalkSpeed = Movement->MaxWalkSpeed;
	}

	Movement->MaxWalkSpeed = OriginalMaxWalkSpeed * MovementSpeedModifier;
	bIsCarrying = true;

	UE_LOG(LogTemp, Log, TEXT("[Battery] Movement penalty applied: %.2f -> %.2f"),
		OriginalMaxWalkSpeed, Movement->MaxWalkSpeed);
}

void AEquippableTool_Battery::RemoveMovementPenalty()
{
	if (!OwningCharacter) return;

	UCharacterMovementComponent* Movement = OwningCharacter->GetCharacterMovement();
	if (!Movement) return;

	if (OriginalMaxWalkSpeed > 0.0f)
	{
		Movement->MaxWalkSpeed = OriginalMaxWalkSpeed;
	}

	bIsCarrying = false;

	UE_LOG(LogTemp, Log, TEXT("[Battery] Movement penalty removed: %.2f"), Movement->MaxWalkSpeed);
}

void AEquippableTool_Battery::ServerApplyMovementPenalty_Implementation()
{
	ApplyMovementPenalty();
}

bool AEquippableTool_Battery::ServerApplyMovementPenalty_Validate()
{
	return true;
}

void AEquippableTool_Battery::ServerRemoveMovementPenalty_Implementation()
{
	RemoveMovementPenalty();
}

bool AEquippableTool_Battery::ServerRemoveMovementPenalty_Validate()
{
	return true;
}
