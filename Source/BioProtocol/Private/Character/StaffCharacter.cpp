// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/StaffCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/StaffStatusComponent.h"
#include "EnhancedInputComponent.h"
#include "Net/UnrealNetwork.h"
#include <Character/MyPlayerController.h>
#include <Kismet/GameplayStatics.h>

#include "BioProtocol/Public/Inventory/InventoryComponent.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "BioProtocol/Public/World/PickUp.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "BioProtocol/Public/Equippable/EquippableUtility/EquippableUtility.h"
#include "BioProtocol/Public/Equippable/EquippableWeapon/EquippableWeapon_.h"
#include "BioProtocol/Public/Equippable/EquippableTool/EquippableTool_Wrench.h"
#include "BioProtocol/Public/Equippable/EquippableTool/EquippableTool_Battery.h"
#include "BioProtocol/Public/Equippable/EquippableTool/EquippableTool_Welder.h"

// Sets default values
AStaffCharacter::AStaffCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;


	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->bUsePawnControlRotation = true;


	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(FirstPersonCamera);

	// ���ο��Ը� ���̱�
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->bCastDynamicShadow = false;
	FirstPersonMesh->CastShadow = false;

	// 3��Ī �޽��� ���ο��� �� ���̰�
	GetMesh()->SetOwnerNoSee(true);

	// ������ �ɼ�
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(44.f);

	Status = CreateDefaultSubobject<UStaffStatusComponent>(TEXT("StatusComponent"));

	InteractionCheckFrequency = 0.1f;
	//ĳ������ ũ�⺸�� ���� �� ���??
	InteractionCheckDistance = 225.f;

	//�ʱ�ȭ
	CurrentEquippedItem = nullptr;
}

// Called when the game starts or when spawned
void AStaffCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled() == true)
	{
		GetMesh()->SetSkeletalMesh(StaffArmMesh);

		APlayerController* PC = Cast<APlayerController>(GetController());
		checkf(IsValid(PC) == true, TEXT("PlayerController is invalid."));

		UEnhancedInputLocalPlayerSubsystem* EILPS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
		checkf(IsValid(EILPS) == true, TEXT("EnhancedInputLocalPlayerSubsystem is invalid."));

		EILPS->AddMappingContext(InputMappingContext, 0);
	}

	if (HasAuthority() && Status)
	{
		Status->ApplyBaseStatus();
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRenderCustomDepth(true);

		MeshComp->SetCustomDepthStencilValue(1);
	}
}

// Called every frame
void AStaffCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	{
		PerformInteractionCheck();
	}
}

// Called to bind functionality to input
void AStaffCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMoveInput);

	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLookInput);

	EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::OnJump);
	EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::OnStopJump);

	EIC->BindAction(RunAction, ETriggerEvent::Triggered, this, &ThisClass::HandleStartRun);
	EIC->BindAction(RunAction, ETriggerEvent::Completed, this, &ThisClass::HandleStopRun);

	/*EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AStaffCharacter::HandleCrouch);
	EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AStaffCharacter::HandleStand);*/

	EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &ThisClass::AttackInput);

	EIC->BindAction(TestKillAction, ETriggerEvent::Started, this, &ThisClass::TestHit);

	//EIC->BindAction(TestPullLever, ETriggerEvent::Triggered, this, &ThisClass::BeginInteract);
	EIC->BindAction(TestPullLever, ETriggerEvent::Started, this, &ThisClass::InteractPressed);
	//EIC->BindAction(TestPullLever, ETriggerEvent::Completed, this, &ThisClass::EndInteract);
	EIC->BindAction(TestPullLever, ETriggerEvent::Completed, this, &ThisClass::InteractReleased);

	EIC->BindAction(Item1, ETriggerEvent::Started, this, &ThisClass::EquipSlot1);
	EIC->BindAction(Item2, ETriggerEvent::Started, this, &ThisClass::EquipSlot2);
	EIC->BindAction(Item3, ETriggerEvent::Started, this, &ThisClass::EquipSlot3);

}

void AStaffCharacter::OnDeath()
{
	/*if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AMyPlayerController* MyPC = Cast<AMyPlayerController>(PC))
		{
			MyPC->ClientStartSpectate();
		}
	}*/
}

void AStaffCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AStaffCharacter, MaterialIndex);
	DOREPLIFETIME(AStaffCharacter, bIsGunEquipped);
	DOREPLIFETIME(AStaffCharacter, CurrentEquippedItem);
	DOREPLIFETIME(AStaffCharacter, CurrentSlot);

}

void AStaffCharacter::SetMaterialByIndex(int32 NewIndex)
{
	if (HasAuthority())
	{
		MaterialIndex = NewIndex;

		if (GetNetMode() != NM_DedicatedServer)
		{
			OnRep_MaterialIndex();
		}
	}
}

void AStaffCharacter::HandleMoveInput(const FInputActionValue& InValue)
{
	if (IsValid(Controller) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("Controller is invalid."));
		return;
	}
	if (bHoldingLever) {
		return;
	}

	const FVector2D InMovementVector = InValue.Get<FVector2D>();

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator ControlYawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	const FVector ForwardDirection = FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, InMovementVector.X);
	AddMovementInput(RightDirection, InMovementVector.Y);
}

void AStaffCharacter::HandleLookInput(const FInputActionValue& InValue)
{
	if (IsValid(Controller) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("Controller is invalid."));
		return;
	}

	const FVector2D InLookVector = InValue.Get<FVector2D>();

	if (bHoldingLever)
	{
		const float CurrentYaw = Controller->GetControlRotation().Yaw;
		const float NextYaw = CurrentYaw + InLookVector.X;
		const float DeltaFromBase = FMath::FindDeltaAngleDegrees(LeverBaseYaw, NextYaw);

		const float MaxAngle = 80.f;
		float Scale = 1.f;

		if (FMath::Abs(DeltaFromBase) > MaxAngle * 0.4f)
		{
			float t = (FMath::Abs(DeltaFromBase) - MaxAngle * 0.4f) / (MaxAngle * 0.4f);
			Scale = FMath::InterpEaseInOut(1.f, 0.f, t, 2.f);
		}

		AddControllerYawInput(InLookVector.X * Scale);
	}
	else
	{
		AddControllerYawInput(InLookVector.X);
	}
	AddControllerPitchInput(InLookVector.Y);
}

void AStaffCharacter::HandleStartRun(const FInputActionValue& InValue)
{
	ServerStartRun();
}
void AStaffCharacter::HandleStopRun(const FInputActionValue& InValue)
{
	if (!HasAuthority())
	{
		ServerStopRun();
		return;
	}

}

void AStaffCharacter::HandleCrouch(const FInputActionValue& InValue)
{
	if (IsLocallyControlled())
	{
		Crouch();
	}

	if (!HasAuthority())
	{
		ServerCrouch();
	}
}

void AStaffCharacter::HandleStand(const FInputActionValue& InValue)
{
	if (IsLocallyControlled()) {
		UnCrouch();
	}

	if (!HasAuthority())
	{
		ServerUnCrouch();
	}
}

void AStaffCharacter::AttackInput(const FInputActionValue& InValue)
{
	if (bHoldingLever) {
		return;
	}
	if (!GetCharacterMovement()->IsFalling())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_None);
		/*FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([&]() -> void
			{
				bCanAttack = true;
				GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}), MeleeAttackMontagePlayTime, false);

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (IsValid(AnimInstance) == true)
		{
			AnimInstance->Montage_Play(MeleeAttackMontage);
		}*/

		/*UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (IsValid(AnimInstance) == true)
		{
			AnimInstance->Montage_Play(MeleeAttackMontage);
		}*/

		ServerRPCMeleeAttack();
	}
}



void AStaffCharacter::PullLever()
{
	if (!IsLocallyControlled())
		return;

	if (!bHoldingLever)
	{
		bHoldingLever = true;
		LeverBaseYaw = GetControlRotation().Yaw;
		ServerPullLever(); //ServerPullLever_Implementation
	}
}
void AStaffCharacter::ServerPullLever_Implementation()
{
	ServerPullLever_Internal();
}

void AStaffCharacter::ServerPullLever_Internal()
{
	AController* C = GetController();
	if (!C)
		return;
	//ktodo:��������(������ ���°Ű�����) �߰��ʿ�
	//UE_LOG(LogTemp, Warning, TEXT("test"));
	if (!GetWorld()->GetTimerManager().IsTimerActive(GaugeTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			GaugeTimerHandle,
			this,
			&AStaffCharacter::TestUpdateLeverGauge,
			0.1f,   
			true    
		);
	}

}


void AStaffCharacter::TestUpdateLeverGauge()
{
	TestGuage = FMath::Clamp(TestGuage + 1.f, 0.f, 100.f);

	if (TestGuage >= 100.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(GaugeTimerHandle);
	}
}

void AStaffCharacter::TestItemSlot1()
{	
	if (HasAuthority())
	{
		bIsGunEquipped = !bIsGunEquipped;
	}
	else
	{
		ServerTestItemSlot1();
	}
}

void AStaffCharacter::ServerTestItemSlot1_Implementation()
{
	bIsGunEquipped = !bIsGunEquipped;

}

void AStaffCharacter::ReleaseLever()
{
	if (!IsLocallyControlled())
		return;

	bHoldingLever = false;


	//���� ���� ������ ������
	FRotator CurrentRot = Controller->GetControlRotation();
	CurrentRot.Yaw = LeverBaseYaw;
	Controller->SetControlRotation(CurrentRot);

	ServerReleaseLever();
}

void AStaffCharacter::ServerReleaseLever_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(GaugeTimerHandle);
}

void AStaffCharacter::ServerRPCMeleeAttack_Implementation()
{
	MulticastRPCMeleeAttack();
}

bool AStaffCharacter::ServerRPCMeleeAttack_Validate()
{
	return true;
}

void AStaffCharacter::ServerStartRun_Implementation()
{
	if (Status->CurrentStamina <= 0.f || !Status->bIsRunable)
	{
		ServerStopRun();
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed =
		Status->MoveSpeed * Status->RunMultiply;

	Status->StartConsumeStamina();
}

void AStaffCharacter::ServerStopRun_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = Status->MoveSpeed;
	if (Status)
	{
		Status->StartRegenStamina();
	}
}

void AStaffCharacter::ServerCrouch_Implementation()
{
	Crouch();
}

void AStaffCharacter::ServerUnCrouch_Implementation()
{
	UnCrouch();
}

void AStaffCharacter::MulticastRPCMeleeAttack_Implementation()
{
	PlayMeleeAttackMontage(MeleeAttackMontage);
}

void AStaffCharacter::OnJump()
{
	if (bHoldingLever) {
		return;
	}
	if (Status->CurrentStamina <= 0.f)
		return;

	ACharacter::Jump();
	ServerOnJump();
}

void AStaffCharacter::OnStopJump()
{
	ACharacter::StopJumping();
	ServerStopJump();
}

void AStaffCharacter::ServerStopJump_Implementation()
{
	ACharacter::StopJumping();
	Status->StartRegenStamina();
}

void AStaffCharacter::ServerOnJump_Implementation()
{
	if (Status->CurrentStamina <= 0.f)
	{
		ACharacter::StopJumping();
		return;
	}

	Status->ConsumeJumpStamina();
}

void AStaffCharacter::PlayMeleeAttackMontage(UAnimMontage* Montage)
{
	if (IsLocallyControlled())
	{
		UAnimInstance* FPAnim = FirstPersonMesh->GetAnimInstance();
		if (FPAnim)
		{
			FPAnim->Montage_Play(Montage);
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (IsValid(AnimInstance) == true)
	{
		AnimInstance->Montage_Play(Montage);
	}
}

float AStaffCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority()) return 0.f;

	const float ActualDamage =
		Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage <= 0.f) return 0.f;

	if (Status)
	{
		Status->ApplyDamage(ActualDamage);
	}

	return ActualDamage;
}

void AStaffCharacter::TestHit()
{
	// ������ ��û
	Server_TestHit();
}

void AStaffCharacter::OnRep_MaterialIndex()
{
	if (IsLocallyControlled())
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::FromInt(MaterialIndex));
	if (mat.IsValidIndex(MaterialIndex))
	{
		GetMesh()->SetMaterial(0, mat[MaterialIndex]);
	}
}


void AStaffCharacter::Server_TestHit_Implementation()
{
	FVector Start = GetActorLocation();
	FVector End = Start + GetActorForwardVector() * 500.f;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Pawn,
		Params
	);

	if (!bHit) return;

	AActor* HitActor = Hit.GetActor();
	if (!HitActor) 	return;

	UE_LOG(LogTemp, Warning, TEXT("Hit %s"), *HitActor->GetName());

	UGameplayStatics::ApplyDamage(
		HitActor,
		50,
		GetController(),
		this,
		UDamageType::StaticClass()
	);
	//Multicast_SetTestMaterial();

}
void AStaffCharacter::Multicast_SetTestMaterial_Implementation()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (mat[0])
	{
		GetMesh()->SetMaterial(0, mat[0]);
	}
}

//void AStaffCharacter::ServerRPCTakeDamage_Implementation(float Damage)
//{
//	if (Status)
//	{
//		Status->ApplyDamage(Damage);
//	}
//}

//bool AStaffCharacter::ServerRPCTakeDamage_Validate(float Damage)
//{
//	return true;
//}

void AStaffCharacter::PerformInteractionCheck()
{
	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	const FVector TraceStart{ GetPawnViewLocation()};
	const FVector TraceEnd{ TraceStart + (GetViewRotation().Vector() * InteractionCheckDistance) };

	//�� �ڿ��� ����Ʈ���̽��� �߻����� �ʵ��� ������ �������� ���
	float LookDirection = FVector::DotProduct(GetActorForwardVector(), GetViewRotation().Vector());

	//���� �ٶ󺸴� �����̶� ���� ���- ���
	if (LookDirection > 0)
	{
		//����, ��, ����, ���Ӽ� ���� �Ҹ�, ����, ���� �켱 ����, �� �β�
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1.f, 0, 2.f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		//����Ʈ���̽��� ����� ���� Hit ����
		FHitResult TraceHit;

		if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
		{
			//��Ʈ�� ã�� �� ���Ͱ� �������̽��� �����ϴ��� Ȯ��
			if (TraceHit.GetActor()->GetClass()->ImplementsInterface(UInteractionInterface::StaticClass()))
			{
				const float Distance = (TraceStart - TraceHit.ImpactPoint).Size();

				if (TraceHit.GetActor() != InteractionData.CurrentInteractable && Distance <= InteractionCheckDistance)
				{
					FoundInteractable(TraceHit.GetActor());
					return;
				}

				if (TraceHit.GetActor() == InteractionData.CurrentInteractable)
				{
					return;
				}
			}
		}
	}
	NoInteractableFound();
}

/*
void AStaffCharacter::NoInteractableFound()
{
	if (IsInteracting())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);
	}
	// �Ⱦ� ����� ��� ���� ���� ���� ���� ���� �� ���
	if (InteractionData.CurrentInteractable)
	{
		TargetInteractable->EndFocus(); 
	}

	//Hide interaction widget on the HUD

	//�ʱ�ȭ
	InteractionData.CurrentInteractable = nullptr;
	TargetInteractable = nullptr;
}
	*/

	void AStaffCharacter::NoInteractableFound()
{
    if (IsInteracting())
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);
    }

    // 이전 포커스 해제
    if (InteractionData.CurrentInteractable)
    {
        if (InteractionData.CurrentInteractable->Implements<UInteractionInterface>())
        {
            IInteractionInterface::Execute_EndFocus(InteractionData.CurrentInteractable);
        }
    }

    // 초기화
    InteractionData.CurrentInteractable = nullptr;
    TargetInteractable.SetObject(nullptr);
    TargetInteractable.SetInterface(nullptr);
}

/*
void AStaffCharacter::BeginInteract()
{
	// verify nothig has changed with the interactavle state since beginning interaction
	PerformInteractionCheck();

	if (InteractionData.CurrentInteractable)
	{
		TargetInteractable->BeginInteract();

		//���ӽð��� ���� 0����
		if (FMath::IsNearlyZero(TargetInteractable->InteractableData.InteractionDuration, 0.1f))
		{
			Interact();
		}
		else
		{
			//���ӽð��� ���� 0�� �ƴ϶�� Ÿ�̸Ӹ� �����ϰ� ������ ��ȣ�ۿ��� ��
			GetWorldTimerManager().SetTimer(TimerHandle_Interaction,
				this,
				&AStaffCharacter::Interact,
				TargetInteractable->InteractableData.InteractionDuration,
				false
			);
		}
	}
}
	*/
void AStaffCharacter::BeginInteract()
{
    // 상호작용 가능 여부 재확인
    PerformInteractionCheck();

    if (!InteractionData.CurrentInteractable)
    {
        return;
    }

    if (!TargetInteractable.GetInterface())
    {
        UE_LOG(LogTemp, Error, TEXT("[Player] TargetInteractable interface is null!"));
        return;
    }

    // BeginInteract 호출
    TargetInteractable->BeginInteract();

    // 상호작용 지속 시간 확인
    FInteractableData Data = TargetInteractable->GetInteractableData();
    
    if (FMath::IsNearlyZero(Data.InteractionDuration, 0.1f))
    {
        Interact();
    }
    else
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_Interaction,
            this,
            &AStaffCharacter::Interact,
            Data.InteractionDuration,
            false
        );
    }
}

void AStaffCharacter::EquipItem(AEquippableItem* Item)
{
	if (!HasAuthority())
	{
		ServerEquipItem(Item);
		return;
	}

	if (!Item)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] EquipItem: Item is null"));
		return;
	}

	// �̹� ������ �������� ������ ���� ����
	if (CurrentEquippedItem)
	{
		UnequipCurrentItem();
	}

	// ������ �ʱ�ȭ (���� �� �Ǿ� �ִٸ�)
	if (!Item->OwningCharacter)
	{
		// TODO: ItemBase ������ �ʿ� �� ����
		Item->Initialize(nullptr, this);
	}

	// ������ ����
	Item->Equip();
	CurrentEquippedItem = Item;

	UE_LOG(LogTemp, Log, TEXT("[Player] Item equipped: %s"), *Item->GetName());
}

void AStaffCharacter::UnequipCurrentItem()
{
	if (!HasAuthority())
	{
		ServerUnequipItem();
		return;
	}

	if (!CurrentEquippedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] No item to unequip"));
		return;
	}

	CurrentEquippedItem->Unequip();
	CurrentEquippedItem = nullptr;

	UE_LOG(LogTemp, Log, TEXT("[Player] Item unequipped"));
}

void AStaffCharacter::DropCurrentItem()
{
	if (!HasAuthority())
	{
		ServerDropItem();
		return;
	}

	if (!CurrentEquippedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] No item to drop"));
		return;
	}

	// ������ �и�
	CurrentEquippedItem->Detach();

	// ������ ������
	FVector ThrowDirection = GetActorForwardVector();
	FVector ThrowLocation = GetActorLocation() + (ThrowDirection * 100.0f);
	CurrentEquippedItem->SetActorLocation(ThrowLocation);

	// ���� Ȱ��ȭ (������ ȿ��)
	if (CurrentEquippedItem->ItemMesh)
	{
		CurrentEquippedItem->ItemMesh->SetSimulatePhysics(true);
		CurrentEquippedItem->ItemMesh->AddImpulse(ThrowDirection * 500.0f, NAME_None, true);
	}

	UE_LOG(LogTemp, Log, TEXT("[Player] Dropped item: %s"), *CurrentEquippedItem->GetName());

	// ���� ����
	CurrentEquippedItem = nullptr;
	CurrentSlot = 0;

	// �κ��丮������ ������ ����
	if (UInventoryComponent* InventoryComp = GetInventory())
	{
		if (CurrentEquippedItem->ItemReference)
		{
			InventoryComp->RemoveItem(CurrentEquippedItem->ItemReference);
		}
	}

}

void AStaffCharacter::UseEquippedItem()
{
	if (!CurrentEquippedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] No item equipped"));
		return;
	}

	// �������� ��� �������� Ȯ��
	if (!CurrentEquippedItem->CanUse())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] Item cannot be used"));
		return;
	}

	// ���� ���: ��� �������� Use() �Լ��� ������ ����
	CurrentEquippedItem->Use();

	UE_LOG(LogTemp, Log, TEXT("[Player] Used item: %s"), *CurrentEquippedItem->GetName());

	// ===== ���� Ÿ�� üũ ��� (���û���) =====
	// Ư�� Ÿ�Ժ��� �ٸ� ó���� �ʿ��� ��쿡�� ���

	// ���� (Tools)
	if (AEquippableTool_Wrench* Wrench = Cast<AEquippableTool_Wrench>(CurrentEquippedItem))
	{
		Wrench->Use();
	}
	else if (AEquippableTool_Welder* Welder = Cast<AEquippableTool_Welder>(CurrentEquippedItem))
	{
		Welder->Use();
	}
	else if (AEquippableTool_Battery* Battery = Cast<AEquippableTool_Battery>(CurrentEquippedItem))
	{
		Battery->Use();
	}
	// ���� (Weapons)
	else if (AEquippableWeapon_* Weapon = Cast<AEquippableWeapon_>(CurrentEquippedItem))
	{
		Weapon->Attack();  // ����� ����
	}
	// ��ƿ��Ƽ (Utilities)
	else if (AEquippableUtility* Utility = Cast<AEquippableUtility>(CurrentEquippedItem))
	{
		Utility->UseUtility();
	}
	// �⺻ ó��
	else
	{
		CurrentEquippedItem->Use();
	}
}

void AStaffCharacter::StopUsingEquippedItem()
{
	if (!CurrentEquippedItem)
	{
		return;
	}

	CurrentEquippedItem->StopUsing();

	UE_LOG(LogTemp, Log, TEXT("[Player] Stopped using item: %s"), *CurrentEquippedItem->GetName());
}

void AStaffCharacter::ReloadWeapon()
{
	if (!CurrentEquippedItem)
	{
		return;
	}

	// �������� Ȯ��
	AEquippableWeapon_* Weapon = Cast<AEquippableWeapon_>(CurrentEquippedItem);
	if (Weapon)
	{
		Weapon->Reload();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] Cannot reload - not a weapon"));
	}
}

void AStaffCharacter::SwitchToSlot(int32 SlotNumber)
{
	if (!Inventory || SlotNumber < 1 || SlotNumber > 3)
	{
		return;
	}

	// ���� ���� �����̸� ����
	if (CurrentSlot == SlotNumber && CurrentEquippedItem)
	{
		return;
	}

	// ���� ������ ����
	if (CurrentEquippedItem)
	{
		Inventory->UnequipCurrentItem();
	}

	// �� ������ ������ ����
	UItemBase* ItemInSlot = Inventory->GetItemInSlot(SlotNumber);
	if (ItemInSlot)
	{
		Inventory->EquipItem(ItemInSlot);
		CurrentSlot = SlotNumber;

		UE_LOG(LogTemp, Log, TEXT("[Player] Equipped slot %d: %s"),
			SlotNumber, *ItemInSlot->TextData.Name.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] Slot %d is empty"), SlotNumber);
		CurrentSlot = 0;
	}
}

void AStaffCharacter::EquipSlot1()
{
	SwitchToSlot(1);
}

void AStaffCharacter::EquipSlot2()
{
	SwitchToSlot(2);
}

void AStaffCharacter::EquipSlot3()
{
	SwitchToSlot(3);
}

void AStaffCharacter::InteractPressed()
{
	if (InteractionData.CurrentInteractable)
	{
		BeginInteract();
	}
}

void AStaffCharacter::InteractReleased()
{
	// EŰ�� ���� ��ȣ�ۿ� ���
	if (InteractionData.CurrentInteractable)
	{
		EndInteract();
	}
}

void AStaffCharacter::DropItemFromInventory(UItemBase* ItemToDrop, int32 QuantityToDrop)
{
	if (!ItemToDrop || !Inventory)
	{
		return;
	}

	// �������� ����
	if (!HasAuthority())
	{
		// ServerDropItem RPC ȣ��
		return;
	}

	// �κ��丮���� ����
	if (QuantityToDrop >= ItemToDrop->Quantity)
	{
		Inventory->RemoveItem(ItemToDrop);
	}
	else
	{
		Inventory->RemoveItemByQuantity(ItemToDrop, QuantityToDrop);
	}

	// ���忡 APickUp ����
	FVector DropLocation = GetActorLocation() + (GetActorForwardVector() * 100.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	APickUp* DroppedPickup = GetWorld()->SpawnActor<APickUp>(
		APickUp::StaticClass(),
		DropLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (DroppedPickup)
	{
		DroppedPickup->InitializeDrop(ItemToDrop, QuantityToDrop);

		// ���� Ȱ��ȭ (������ ȿ��)
		if (DroppedPickup->PickUpMesh)
		{
			DroppedPickup->PickUpMesh->SetSimulatePhysics(true);
			DroppedPickup->PickUpMesh->AddImpulse(GetActorForwardVector() * 500.0f);
		}
	}
}

bool AStaffCharacter::HasRequiredTool(FName ToolID)
{
	if (!Inventory)
	{
		return false;
	}

	UItemBase* Tool = Inventory->FindItemByID(ToolID);
	return Tool != nullptr;
}

void AStaffCharacter::GiveStartingItems()
{
	if (!HasAuthority() || !Inventory)
	{
		return;
	}

	// ��ġ ����
	UItemBase* Wrench = CreateItemFromDataTable(FName("Wrench"), 1);
	if (Wrench)
	{
		Inventory->HandleAddItem(Wrench);
		UE_LOG(LogTemp, Log, TEXT("[Player] Starting items given"));
	}
}

/*
void AStaffCharacter::FoundInteractable(AActor* NewInteractable)
{
	//�ð� ���� ��ȣ�ۿ� ���߿� ���ο� ��ü�� �߰���
	if (IsInteracting())
	{
		EndInteract();
	}
	//�����Ϳ� �̹� ��ȣ�ۿ� ������ ��ü�� �ִٸ� ���� ��ü�� �����Դ�
	if (InteractionData.CurrentInteractable)
	{
		TargetInteractable = InteractionData.CurrentInteractable;
		//���Ϳ��� �ü��� ����� �� �˸���-��������/ ���̶���Ʈ ��
		TargetInteractable->EndFocus();
	}

	InteractionData.CurrentInteractable = NewInteractable;
	TargetInteractable = NewInteractable;

	TargetInteractable->BeginFocus();
}
	*/

	void AStaffCharacter::FoundInteractable(AActor* NewInteractable)
{
    if (!NewInteractable)
    {
        UE_LOG(LogTemp, Error, TEXT("[Player] NewInteractable is null!"));
        return;
    }

    // Interface 확인
    if (!NewInteractable->Implements<UInteractionInterface>())
    {
        UE_LOG(LogTemp, Error, TEXT("[Player] Actor does not implement IInteractionInterface: %s"), 
            *NewInteractable->GetName());
        return;
    }

    // 시간 제한 상호작용 도중에 새로운 객체를 발견함
    if (IsInteracting())
    {
        EndInteract();
    }

    // 이전 오브젝트 포커스 해제
    if (InteractionData.CurrentInteractable && InteractionData.CurrentInteractable != NewInteractable)
    {
        if (InteractionData.CurrentInteractable->Implements<UInteractionInterface>())
        {
            IInteractionInterface::Execute_EndFocus(InteractionData.CurrentInteractable);
        }
    }

    // 새 오브젝트 설정
    InteractionData.CurrentInteractable = NewInteractable;
    TargetInteractable.SetObject(NewInteractable);  
    TargetInteractable.SetInterface(Cast<IInteractionInterface>(NewInteractable));  

    // BeginFocus 호출
    if (TargetInteractable.GetInterface())
    {
        TargetInteractable->BeginFocus();
        UE_LOG(LogTemp, Log, TEXT("[Player] Found interactable: %s"), *NewInteractable->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Player] Failed to get interface from: %s"), *NewInteractable->GetName());
    }
}


void AStaffCharacter::PlayToolUseMontage(UAnimMontage* Montage)
{
	if (!Montage)
		return;

	if (USkeletalMeshComponent* CharMesh = GetMesh())
	{
		if (UAnimInstance* AnimInstance = CharMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Play(Montage, 1.0f);
		}
	}
}

/*
void AStaffCharacter::EndInteract()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	if (IsValid(TargetInteractable.GetObject()))
	{
		TargetInteractable->EndInteract();
	}
}
	*/

	
void AStaffCharacter::EndInteract()
{
    GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

    if (TargetInteractable.GetInterface())
    {
        TargetInteractable->EndInteract();
    }
}

/*
void AStaffCharacter::Interact()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	if (TargetInteractable.GetInterface() != nullptr)
	{
		IInteractionInterface::Execute_Interact(TargetInteractable.GetObject(), this);
	}
} */

void AStaffCharacter::Interact()
{
    GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

    if (TargetInteractable.GetInterface())
    {
        IInteractionInterface::Execute_Interact(TargetInteractable.GetObject(), this);
        UE_LOG(LogTemp, Log, TEXT("[Player] Interact executed"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Player] Cannot interact - interface is null"));
    }
}

UItemBase* AStaffCharacter::CreateItemFromDataTable(FName ItemID, int32 Quantity)
{
	if (!ItemDataTable || ItemID.IsNone())
	{
		return nullptr;
	}

	const FItemData* ItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));
	if (!ItemData)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] Item not found: %s"), *ItemID.ToString());
		return nullptr;
	}

	UItemBase* NewItem = NewObject<UItemBase>(this, UItemBase::StaticClass());
	if (!NewItem)
	{
		return nullptr;
	}

	// ������ ����
	NewItem->ItemID = ItemData->ItemID;
	NewItem->ItemType = ItemData->ItemType;
	NewItem->ItemQuality = ItemData->ItemQuality;
	NewItem->Category = ItemData->Category;
	NewItem->NumericData = ItemData->NumericData;
	NewItem->TextData = ItemData->TextData;
	NewItem->AssetData = ItemData->AssetData;
	NewItem->ItemClass = ItemData->ItemClass;
	NewItem->SetQuantity(FMath::Max(Quantity, 1));

	return NewItem;
}

void AStaffCharacter::ServerEquipItem_Implementation(AEquippableItem* Item)
{
	EquipItem(Item);
}

bool AStaffCharacter::ServerEquipItem_Validate(AEquippableItem* Item)
{
	return Item != nullptr;
}
void AStaffCharacter::ServerUnequipItem_Implementation()
{
	UnequipCurrentItem();
}

bool AStaffCharacter::ServerUnequipItem_Validate()
{
	return true;
}

void AStaffCharacter::ServerDropItem_Implementation()
{
	DropCurrentItem();
}

bool AStaffCharacter::ServerDropItem_Validate()
{
	return true;
}

void AStaffCharacter::ServerDropCurrentItem_Implementation()
{
	if (CurrentEquippedItem && Inventory)
	{
		// ���� ������ �������� ItemReference ã��
		UItemBase* ItemToDrop = CurrentEquippedItem->ItemReference;
		if (ItemToDrop)
		{
			DropItemFromInventory(ItemToDrop, 1);
		}
	}
}

void AStaffCharacter::ServerUseItem_Implementation()
{
	UseEquippedItem();
}

bool AStaffCharacter::ServerUseItem_Validate()
{
	return CurrentEquippedItem != nullptr;
}

// ---

void AStaffCharacter::ServerStopUsingItem_Implementation()
{
	StopUsingEquippedItem();
}

bool AStaffCharacter::ServerStopUsingItem_Validate()
{
	return true;
}

void AStaffCharacter::Die()
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Player] Died"));

	//// ��� ó��
	//// 1. ĳ���� ��Ȱ��ȭ
	//GetCharacterMovement()->DisableMovement();

	//// 2. �浹 ��Ȱ��ȭ
	//GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. ���� ���� ������ ���
	if (CurrentEquippedItem)
	{
		DropCurrentItem();
	}

	// 4. �κ��丮 ������ ��� ��� (���û���)
	if (Inventory)
	{
		const TArray<UItemBase*>& AllItems = Inventory->GetAllItems();
		for (UItemBase* Item : AllItems)
		{
			if (Item)
			{
				DropItemFromInventory(Item, Item->Quantity);
			}
		}
	}

	// 5. TODO: ��� �ִϸ��̼� ���
	// 6. TODO: ������ Ÿ�̸� ����

	// ��������Ʈ ��ε�ĳ��Ʈ
	//OnHealthChanged.Broadcast(0.0f, MaxHealth);
}

void AStaffCharacter::OnRep_CurrentEquippedItem()
{
	UE_LOG(LogTemp, Log, TEXT("[Player Client] CurrentEquippedItem updated"));
}
