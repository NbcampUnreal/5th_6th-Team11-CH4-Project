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

	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	InteractionCheckFrequency = 0.1f;
	InteractionCheckDistance = 225.f;

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
	EIC->BindAction(DropAction, ETriggerEvent::Started, this, &ThisClass::DropCurrentItemInput);

	EIC->BindAction(TestItem1, ETriggerEvent::Started, this, &ThisClass::EquipSlot1);
	EIC->BindAction(TestItem2, ETriggerEvent::Started, this, &ThisClass::EquipSlot2);
	EIC->BindAction(TestItem3, ETriggerEvent::Started, this, &ThisClass::EquipSlot3);

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
	DOREPLIFETIME(AStaffCharacter, Inventory);

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

void AStaffCharacter::OnJump(const FInputActionValue& InValue)
{
	if (bHoldingLever) {
		return;
	}
	if (Status->CurrentStamina <= 0.f)
		return;

	ACharacter::Jump();
	ServerOnJump();
}

void AStaffCharacter::OnStopJump(const FInputActionValue& InValue)
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

	const FVector TraceStart{ GetPawnViewLocation() };
	const FVector TraceEnd{ TraceStart + (GetViewRotation().Vector() * InteractionCheckDistance) };

	// 뒤로 돌아볼 때는 LineTrace 발생 안 함
	float LookDirection = FVector::DotProduct(GetActorForwardVector(), GetViewRotation().Vector());

	if (LookDirection > 0)
	{
		// Debug Line
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1.f, 0, 2.f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		FHitResult TraceHit;

		if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
		{
			AActor* HitActor = TraceHit.GetActor();

			// 디버그: Hit한 액터 출력
			UE_LOG(LogTemp, Warning, TEXT("[Player] LineTrace hit: %s"),
				HitActor ? *HitActor->GetName() : TEXT("NULL"));

			// 🔧 수정: Interface 체크를 if 안에서만 하도록 변경
			if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractionInterface::StaticClass()))
			{
				UE_LOG(LogTemp, Log, TEXT("[Player] ✓ Actor implements IInteractionInterface!"));

				const float Distance = (TraceStart - TraceHit.ImpactPoint).Size();

				// 새로운 interactable 발견
				if (HitActor != InteractionData.CurrentInteractable && Distance <= InteractionCheckDistance)
				{
					UE_LOG(LogTemp, Log, TEXT("[Player] New interactable found! Distance: %.2f"), Distance);
					FoundInteractable(HitActor);
					return;  // ← 여기서 함수 종료!
				}

				// 이미 현재 interactable인 경우
				if (HitActor == InteractionData.CurrentInteractable)
				{
					// 계속 바라보고 있음 - 아무것도 안 함
					return;  // ← 여기서 함수 종료!
				}
			}
			else
			{
				// Interface를 구현하지 않은 물체를 봄
				UE_LOG(LogTemp, Log, TEXT("[Player] Hit actor does NOT implement IInteractionInterface"));
			}
		}
		else
		{
			// LineTrace가 아무것도 안 맞음
			UE_LOG(LogTemp, Log, TEXT("[Player] LineTrace hit nothing"));
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
	// 이미 interactable이 없으면 아무것도 안 함
	if (!InteractionData.CurrentInteractable)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Player] No interactable found, clearing current"));

	// 진행 중인 타이머 제거
	if (IsInteracting())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);
	}

	// 이전 포커스 해제
	if (InteractionData.CurrentInteractable->Implements<UInteractionInterface>())
	{
		IInteractionInterface::Execute_EndFocus(InteractionData.CurrentInteractable);
	}

	// 초기화
	InteractionData.CurrentInteractable = nullptr;

	// 수정: TargetInteractable 사용 안 함 (제거 권장)
	// TargetInteractable.SetObject(nullptr);
	// TargetInteractable.SetInterface(nullptr);
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
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[Player] BeginInteract called!"));

	// 🔧 수정: PerformInteractionCheck 제거 (이미 Tick에서 계속 호출됨)
	// PerformInteractionCheck();  // ← 불필요! 제거!

	if (!InteractionData.CurrentInteractable)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] No CurrentInteractable!"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}

	AActor* InteractableActor = InteractionData.CurrentInteractable;

	UE_LOG(LogTemp, Log, TEXT("[Player] CurrentInteractable: %s"), *InteractableActor->GetName());

	// Interface 체크
	if (!InteractableActor->Implements<UInteractionInterface>())
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] Actor does not implement IInteractionInterface!"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}

	// 🔧 수정: BeginInteract 호출
	UE_LOG(LogTemp, Log, TEXT("[Player] Calling Execute_BeginInteract..."));
	IInteractionInterface::Execute_BeginInteract(InteractableActor);

	// 상호작용 데이터 가져오기
	const FInteractableData Data = IInteractionInterface::Execute_GetInteractableData(InteractableActor);

	UE_LOG(LogTemp, Log, TEXT("[Player] Interaction Duration: %.2f"), Data.InteractionDuration);

	// 즉시 상호작용 vs 타이머
	if (FMath::IsNearlyZero(Data.InteractionDuration, 0.1f))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] Instant interaction - calling Interact() immediately!"));
		Interact();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Player] Setting timer for %.2f seconds..."), Data.InteractionDuration);

		// 🔧 수정: Lambda 대신 직접 Interact() 호출하도록 타이머 설정
		GetWorldTimerManager().SetTimer(
			TimerHandle_Interaction,
			this,
			&AStaffCharacter::Interact,
			Data.InteractionDuration,
			false
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
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
	// 서버에서만 실행
	if (!HasAuthority())
	{
		ServerDropItem();
		return;
	}

	// 장착된 아이템이 없으면 리턴
	if (!CurrentEquippedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] No item to drop"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Player] Dropping equipped item: %s"),
		*CurrentEquippedItem->GetName());

	AEquippableItem* EquippedItem = CurrentEquippedItem;
	UItemBase* ItemReference = EquippedItem->ItemReference;

	if (!ItemReference)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] ItemReference is null!"));
		return;
	}

	// ========================================
	// 아이템 던질 위치 계산
	// ========================================
	FVector ThrowDirection = GetActorForwardVector();
	FVector ThrowLocation = GetActorLocation() + (ThrowDirection * 100.0f) + FVector(0, 0, 50.0f);
	FRotator ThrowRotation = GetActorRotation();

	// ========================================
	// 월드에 APickUp 생성
	// ========================================
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APickUp* DroppedPickup = GetWorld()->SpawnActor<APickUp>(
		APickUp::StaticClass(),
		ThrowLocation,
		ThrowRotation,
		SpawnParams
	);

	if (!DroppedPickup)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] Failed to spawn PickUp!"));
		return;
	}

	// ========================================
	// PickUp 초기화
	// ========================================
	DroppedPickup->InitializeDrop(ItemReference, ItemReference->Quantity);

	// ========================================
	// 물리 시뮬레이션 활성화 (던지기)
	// ========================================
	if (DroppedPickup->PickUpMesh)
	{
		DroppedPickup->PickUpMesh->SetSimulatePhysics(true);
		DroppedPickup->PickUpMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DroppedPickup->PickUpMesh->AddImpulse(ThrowDirection * 500.0f, NAME_None, true);

		UE_LOG(LogTemp, Log, TEXT("[Player] PickUp physics enabled and thrown"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] PickUp has no mesh component"));
	}

	// ========================================
	// 현재 장착 해제
	// ========================================
	CurrentEquippedItem = nullptr;
	CurrentSlot = 0;

	// ========================================
	// 인벤토리에서 제거
	// ========================================
	if (Inventory)
	{
		Inventory->RemoveItem(ItemReference);
		UE_LOG(LogTemp, Log, TEXT("[Player] Item removed from inventory"));
	}

	// ========================================
	// AEquippableItem 제거 (중요!)
	// ========================================
	if (EquippedItem)
	{
		EquippedItem->Destroy();
		UE_LOG(LogTemp, Log, TEXT("[Player] EquippableItem destroyed"));
	}

	UE_LOG(LogTemp, Warning, TEXT("[Player] ✓ Drop completed! PickUp spawned: %s"),
		*DroppedPickup->GetName());

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

void AStaffCharacter::EquipSlot1(const FInputActionValue& InValue)
{
	SwitchToSlot(1);
}

void AStaffCharacter::EquipSlot2(const FInputActionValue& InValue)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
		TEXT("===== 2 KEY WORKS! CODE IS LOADED! ====="));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[Player] ===== 2 KEY PRESSED ====="));

	if (!Inventory)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] Inventory is NULL!"));
		return;
	}

	// 인벤토리 전체 출력
	const TArray<UItemBase*>& AllItems = Inventory->GetAllItems();
	UE_LOG(LogTemp, Warning, TEXT("[Player] Total items in inventory: %d"), AllItems.Num());

	for (int32 i = 0; i < AllItems.Num(); i++)
	{
		if (AllItems[i])
		{
			UE_LOG(LogTemp, Log, TEXT("  Item[%d]: %s (Quantity: %d, ItemClass: %s)"),
				i,
				*AllItems[i]->TextData.Name.ToString(),
				AllItems[i]->Quantity,
				AllItems[i]->ItemClass ? *AllItems[i]->ItemClass->GetName() : TEXT("NULL"));
		}
	}

	// Slot 2 확인
	UItemBase* ItemInSlot2 = Inventory->GetItemInSlot(2);
	if (ItemInSlot2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] Slot 2 has: %s"),
			*ItemInSlot2->TextData.Name.ToString());

		if (!ItemInSlot2->ItemClass)
		{
			UE_LOG(LogTemp, Error, TEXT("[Player] ✗ ItemClass is NULL!"));
			UE_LOG(LogTemp, Error, TEXT("[Player] You must set ItemClass in DataTable!"));
			UE_LOG(LogTemp, Warning, TEXT("========================================"));
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("[Player] ItemClass: %s"),
			*ItemInSlot2->ItemClass->GetName());

		UE_LOG(LogTemp, Warning, TEXT("[Player] Calling SwitchToSlot(2)..."));
		SwitchToSlot(2);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] Slot 2 is EMPTY!"));

		// 모든 슬롯 확인
		for (int32 i = 1; i <= 3; i++)
		{
			UItemBase* Item = Inventory->GetItemInSlot(i);
			if (Item)
			{
				UE_LOG(LogTemp, Log, TEXT("[Player] Slot %d: %s"),
					i, *Item->TextData.Name.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[Player] Slot %d: EMPTY"), i);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void AStaffCharacter::EquipSlot3(const FInputActionValue& InValue)
{
	SwitchToSlot(3);
}

void AStaffCharacter::InteractPressed(const FInputActionValue& InValue)
{
	UE_LOG(LogTemp, Warning, TEXT("[Player] ===== E KEY PRESSED ====="));

	if (InteractionData.CurrentInteractable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] CurrentInteractable exists: %s"),
			*InteractionData.CurrentInteractable->GetName());
		BeginInteract();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] No CurrentInteractable! Cannot interact!"));
	}
}

void AStaffCharacter::InteractReleased(const FInputActionValue& InValue)
{
	UE_LOG(LogTemp, Log, TEXT("[Player] E KEY RELEASED"));

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
	FVector DropLocation = GetActorLocation() + (GetActorForwardVector() * 100.0f) + FVector(0, 0, 50.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

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
			DroppedPickup->PickUpMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			DroppedPickup->PickUpMesh->AddImpulse(GetActorForwardVector() * 500.0f, NAME_None, true);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[Player] ✓ PickUp spawned: %s"),
		*DroppedPickup->GetName());
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
		UE_LOG(LogTemp, Error, TEXT("[Player] FoundInteractable: NewInteractable is null!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[Player] FoundInteractable called"));
	UE_LOG(LogTemp, Warning, TEXT("[Player] Actor: %s"), *NewInteractable->GetName());

	// Interface 확인 (안전장치)
	if (!NewInteractable->Implements<UInteractionInterface>())
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] Actor does not implement IInteractionInterface!"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}

	// 시간 제한 상호작용 도중에 새로운 객체를 발견함
	if (IsInteracting())
	{
		UE_LOG(LogTemp, Log, TEXT("[Player] Currently interacting, ending previous interaction"));
		EndInteract();
	}

	// 이전 오브젝트 포커스 해제
	if (InteractionData.CurrentInteractable && InteractionData.CurrentInteractable != NewInteractable)
	{
		UE_LOG(LogTemp, Log, TEXT("[Player] Ending focus on previous object"));
		if (InteractionData.CurrentInteractable->Implements<UInteractionInterface>())
		{
			IInteractionInterface::Execute_EndFocus(InteractionData.CurrentInteractable);
		}
	}

	// 새 오브젝트 설정
	InteractionData.CurrentInteractable = NewInteractable;

	// BeginFocus 호출
	IInteractionInterface::Execute_BeginFocus(NewInteractable);

	UE_LOG(LogTemp, Warning, TEXT("[Player] ✓ BeginFocus called successfully!"));
	UE_LOG(LogTemp, Log, TEXT("[Player] Found interactable: %s"), *NewInteractable->GetName());
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
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

	if (InteractionData.CurrentInteractable &&
		InteractionData.CurrentInteractable->Implements<UInteractionInterface>())
	{
		IInteractionInterface::Execute_EndInteract(InteractionData.CurrentInteractable);
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
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[Player] ★★★ Interact() called! ★★★"));

	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	AActor* InteractableActor = InteractionData.CurrentInteractable;

	if (!InteractableActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] CurrentInteractable is null!"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}

	if (!InteractableActor->Implements<UInteractionInterface>())
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] Actor does not implement IInteractionInterface!"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Player] Calling IInteractionInterface::Execute_Interact..."));
	UE_LOG(LogTemp, Warning, TEXT("[Player] Target Object: %s"), *InteractableActor->GetName());
	UE_LOG(LogTemp, Warning, TEXT("[Player] Player Character: %s"), *GetName());

	// Interact 실행!
	IInteractionInterface::Execute_Interact(InteractableActor, this);

	UE_LOG(LogTemp, Warning, TEXT("[Player] ✓ Execute_Interact completed!"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

UItemBase* AStaffCharacter::CreateItemFromDataTable(FName ItemID, int32 Quantity)
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[CreateItem] CreateItemFromDataTable called"));
	UE_LOG(LogTemp, Warning, TEXT("[CreateItem] ItemID: %s"), *ItemID.ToString());

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] ItemDataTable is NULL!"));
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] Check BP_StaffCharacter → ItemDataTable variable!"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[CreateItem] ItemDataTable: %s"), *ItemDataTable->GetName());

	if (ItemID.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] ItemID is None!"));
		return nullptr;
	}

	// DataTable에서 행 찾기
	const FItemData* ItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));

	if (!ItemData)
	{
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] Item not found in DataTable: %s"), *ItemID.ToString());

		// DataTable의 모든 행 출력
		TArray<FName> RowNames = ItemDataTable->GetRowNames();
		UE_LOG(LogTemp, Warning, TEXT("[CreateItem] Available rows in DataTable:"));
		for (const FName& RowName : RowNames)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - %s"), *RowName.ToString());
		}

		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[CreateItem] ItemData found!"));
	UE_LOG(LogTemp, Log, TEXT("[CreateItem] ItemData->ItemID: %s"), *ItemData->ItemID.ToString());

	// ========================================
	// 🔥 ItemClass 확인 (핵심!)
	// ========================================
	if (!ItemData->ItemClass)
	{
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] ✗✗✗ ItemClass is NULL in DataTable! ✗✗✗"));
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] Row: %s"), *ItemID.ToString());
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] This means the DataTable is NOT updated!"));
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] Solution:"));
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] 1. Close Editor"));
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] 2. Delete Saved/ and Intermediate/ folders"));
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] 3. Restart Editor"));
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] 4. Re-open DataTable and verify ItemClass"));
		UE_LOG(LogTemp, Error, TEXT("========================================"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CreateItem] ✓ ItemClass found: %s"),
			*ItemData->ItemClass->GetName());
	}

	// UItemBase 생성
	UItemBase* NewItem = NewObject<UItemBase>(this, UItemBase::StaticClass());
	if (!NewItem)
	{
		UE_LOG(LogTemp, Error, TEXT("[CreateItem] Failed to create UItemBase!"));
		return nullptr;
	}

	// 데이터 복사
	NewItem->ItemID = ItemData->ItemID;
	NewItem->ItemType = ItemData->ItemType;
	NewItem->ItemQuality = ItemData->ItemQuality;
	NewItem->Category = ItemData->Category;
	NewItem->NumericData = ItemData->NumericData;
	NewItem->TextData = ItemData->TextData;
	NewItem->AssetData = ItemData->AssetData;
	NewItem->ItemClass = ItemData->ItemClass;  // ← 여기서 복사!
	NewItem->SetQuantity(FMath::Max(Quantity, 1));

	UE_LOG(LogTemp, Warning, TEXT("[CreateItem] ✓ NewItem created successfully!"));
	UE_LOG(LogTemp, Warning, TEXT("[CreateItem] NewItem->ItemClass: %s"),
		NewItem->ItemClass ? *NewItem->ItemClass->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));

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
	DropCurrentItem();
}

bool AStaffCharacter::ServerDropCurrentItem_Validate()
{
	return true;
}

void AStaffCharacter::ServerUseItem_Implementation()
{
	UseEquippedItem();
}

bool AStaffCharacter::ServerUseItem_Validate()
{
	return CurrentEquippedItem != nullptr;
}


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

// ========================================
// 4. INPUT HANDLER
// ========================================

void AStaffCharacter::DropCurrentItemInput(const FInputActionValue& InValue)
{
	UE_LOG(LogTemp, Warning, TEXT("[Player] Drop key (G) pressed!"));

	// 장착된 아이템이 없으면 리턴
	if (!CurrentEquippedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] No equipped item to drop"));
		return;
	}

	// 서버에서 실행
	if (!HasAuthority())
	{
		ServerDropItem();
		return;
	}

	DropCurrentItem();
}