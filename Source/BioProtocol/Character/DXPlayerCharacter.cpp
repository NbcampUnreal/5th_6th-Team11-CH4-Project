#include "DXPlayerCharacter.h"

#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"

#include "BioProtocol/Public/Inventory/InventoryComponent.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "BioProtocol/Public/World/PickUp.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "BioProtocol/Public/Equippable/EquippableUtility/EquippableUtility.h"
#include "BioProtocol/Public/Equippable/EquippableWeapon/EquippableWeapon_.h"
#include "BioProtocol/Public/Equippable/EquippableTool/EquippableTool_Wrench.h"
#include "BioProtocol/Public/Equippable/EquippableTool/EquippableTool_Battery.h"
#include "BioProtocol/Public/Equippable/EquippableTool/EquippableTool_Welder.h"


ADXPlayerCharacter::ADXPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	//내가 입력한 방향으로 움직임!!
	//카메라가 보는 방향을 따라갈 건가? 아님
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	//이동하는 방향 쪽으로 몸을 돌릴 건가? 웅
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
 
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->bUsePawnControlRotation = false;
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	InteractionCheckFrequency = 0.1f;
	//캐릭터의 크기보다 조금 더 길게??
	InteractionCheckDistance = 225.f;

	//초기화
	CurrentEquippedItem = nullptr;
}

void ADXPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EIC->BindAction(
		MoveAction,
		ETriggerEvent::Triggered,
		this,
		&ThisClass::HandleMoveInput);
	EIC->BindAction(
		LookAction,
		ETriggerEvent::Triggered,
		this,
		&ThisClass::HandleLookInput);
	EIC->BindAction(
		JumpAction,
		ETriggerEvent::Triggered,
		this,
		&ACharacter::Jump);
	EIC->BindAction(
		JumpAction,
		ETriggerEvent::Completed,
		this,
		&ACharacter::StopJumping);
	
	EIC->BindAction(
		InteractAction,
		ETriggerEvent::Started, 
		this, 
		&ThisClass::BeginInteract);
	EIC->BindAction(
		InteractAction, 
		ETriggerEvent::Completed, 
		this, 
		&ThisClass::EndInteract);

	// 아이템 사용 (마우스 좌클릭)
	PlayerInputComponent->BindAction("UseItem", IE_Pressed, this, &ADXPlayerCharacter::UseEquippedItem);
	PlayerInputComponent->BindAction("UseItem", IE_Released, this, &ADXPlayerCharacter::StopUsingEquippedItem);

	// 재장전 (R키)
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ADXPlayerCharacter::ReloadWeapon);

	// 슬롯 전환 (1, 2, 3 키)
	PlayerInputComponent->BindAction("Slot1", IE_Pressed, this, &ADXPlayerCharacter::EquipSlot1);
	PlayerInputComponent->BindAction("Slot2", IE_Pressed, this, &ADXPlayerCharacter::EquipSlot2);
	PlayerInputComponent->BindAction("Slot3", IE_Pressed, this, &ADXPlayerCharacter::EquipSlot3);
}

void ADXPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocallyControlled() == false)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is null in BeginPlay"));
		return;
	}

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		UE_LOG(LogTemp, Warning, TEXT("LocalPlayer is null in BeginPlay"));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	if (!SubSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnhancedInputLocalPlayerSubsystem is null"));
		return;
	}

	// 4) IMC 추가
	if (InputMappingContext)
	{
		SubSystem->AddMappingContext(InputMappingContext, 0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InputMappingContext is null"));
	}
}

void ADXPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	{
		PerformInteractionCheck();
	}
}

void ADXPlayerCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADXPlayerCharacter, CurrentEquippedItem);
	DOREPLIFETIME(ADXPlayerCharacter, CurrentSlot);
	DOREPLIFETIME(ADXPlayerCharacter, Health);
}

void ADXPlayerCharacter::HandleMoveInput(const FInputActionValue& InValue)
{
	if (IsValid(Controller) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("Controller is invalid."));
		return;
	}
	const FVector2D InMovementVector = InValue.Get<FVector2D>();

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator ControlYawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, InMovementVector.X);
	AddMovementInput(RightDirection, InMovementVector.Y);

}

void ADXPlayerCharacter::HandleLookInput(const FInputActionValue& InValue)
{
	if (IsValid(Controller) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("Controller is invalid."));
		return;
	}
	const FVector2D InLookVector = InValue.Get<FVector2D>();
	AddControllerYawInput(InLookVector.X);
	AddControllerPitchInput(InLookVector.Y);
}



void ADXPlayerCharacter::PerformInteractionCheck()
{
	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	const FVector TraceStart{ GetPawnViewLocation()};
	const FVector TraceEnd{ TraceStart + (GetViewRotation().Vector() * InteractionCheckDistance) };

	//목 뒤에서 라인트레이스가 발생되지 않도록 벡터의 내적으로 계산
	float LookDirection = FVector::DotProduct(GetActorForwardVector(), GetViewRotation().Vector());

	//폰이 바라보는 방향이랑 같을 경우- 양수
	if (LookDirection > 0)
	{
		//시작, 끝, 빨강, 지속선 유지 할말, 수명, 깊이 우선 순위, 선 두께
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1.f, 0, 2.f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		//라인트레이스의 결과를 담을 Hit 정보
		FHitResult TraceHit;

		if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
		{
			//히트를 찾은 후 액터가 인터페이스를 구현하는지 확인
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

void ADXPlayerCharacter::FoundInteractable(AActor* NewInteractable)
{
	//시간 제한 상호작용 도중에 새로운 객체를 발견함
	if (IsInteracting())
	{
		EndInteract();
	}
	//데이터에 이미 상호작용 가능한 객체가 있다면 현재 객체로 설정함다
	if (InteractionData.CurrentInteractable)
	{
		TargetInteractable = InteractionData.CurrentInteractable;
		//액터에서 시선이 벗어났을 때 알리기-위젯끄기/ 하이라이트 등
		TargetInteractable->EndFocus();
	}

	InteractionData.CurrentInteractable = NewInteractable;
	TargetInteractable = NewInteractable;

	TargetInteractable->BeginFocus();
}

void ADXPlayerCharacter::NoInteractableFound()
{
	if (IsInteracting())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);
	}
	// 픽업 기능의 경우 무기 슬롯 수량 제한 있을 때 사용
	if (InteractionData.CurrentInteractable)
	{
		TargetInteractable->EndFocus(); 
	}

	//Hide interaction widget on the HUD

	//초기화
	InteractionData.CurrentInteractable = nullptr;
	TargetInteractable = nullptr;

}

void ADXPlayerCharacter::BeginInteract()
{
	// verify nothig has changed with the interactavle state since beginning interaction
	PerformInteractionCheck();

	if (InteractionData.CurrentInteractable)
	{
		TargetInteractable->BeginInteract();

		//지속시간이 거의 0인지
		if (FMath::IsNearlyZero(TargetInteractable->InteractableData.InteractionDuration, 0.1f))
		{
			Interact();
		}
		else
		{
			//지속시간이 거의 0이 아니라면 타이머를 시작하고 끝나면 상호작용을 함
			GetWorldTimerManager().SetTimer(TimerHandle_Interaction,
				this,
				&ADXPlayerCharacter::Interact,
				TargetInteractable->InteractableData.InteractionDuration,
				false
			);
		}
	}
}

void ADXPlayerCharacter::EndInteract()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	if (IsValid(TargetInteractable.GetObject()))
	{
		TargetInteractable->EndInteract();
	}
}

void ADXPlayerCharacter::Interact()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	if (TargetInteractable.GetInterface() != nullptr)
	{
		IInteractionInterface::Execute_Interact(TargetInteractable.GetObject(), this);
	}
}

// ===== 상호작용 (E키) =====

void ADXPlayerCharacter::InteractPressed()
{
	if (CurrentInteractable)
	{
		// 상호작용 시작
		IInteractionInterface* Interactable = Cast<IInteractionInterface>(CurrentInteractable);
		if (Interactable)
		{
			Interactable->Execute_BeginInteract(CurrentInteractable);

			// 상호작용 데이터 가져오기
			FInteractableData Data = Interactable->Execute_GetInteractableData(CurrentInteractable);

			// 즉시 실행 (Duration이 0이면)
			if (Data.InteractionDuration <= 0.0f)
			{
				Interactable->Execute_Interact(CurrentInteractable, this);
			}
			else
			{
				// 타이머로 지연 실행
				GetWorldTimerManager().SetTimer(
					InteractionTimerHandle,
					[this, Interactable]()
					{
						Interactable->Execute_Interact(CurrentInteractable, this);
					},
					Data.InteractionDuration,
					false
				);
			}
		}
	}
}

void ADXPlayerCharacter::InteractReleased()
{
	// E키를 떼면 상호작용 취소
	if (CurrentInteractable)
	{
		GetWorldTimerManager().ClearTimer(InteractionTimerHandle);

		IInteractionInterface* Interactable = Cast<IInteractionInterface>(CurrentInteractable);
		if (Interactable)
		{
			Interactable->Execute_EndInteract(CurrentInteractable);
		}
	}
}


void ADXPlayerCharacter::EquipItem(AEquippableItem* Item)
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

	// 이미 장착된 아이템이 있으면 먼저 해제
	if (CurrentEquippedItem)
	{
		UnequipCurrentItem();
	}

	// 아이템 초기화 (아직 안 되어 있다면)
	if (!Item->OwningCharacter)
	{
		// TODO: ItemBase 데이터 필요 시 생성
		Item->Initialize(nullptr, this);
	}

	// 아이템 장착
	Item->Equip();
	CurrentEquippedItem = Item;

	UE_LOG(LogTemp, Log, TEXT("[Player] Item equipped: %s"), *Item->GetName());
}

void ADXPlayerCharacter::UnequipCurrentItem()
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

// ===== 재장전 =====

void ADXPlayerCharacter::ReloadWeapon()
{
	if (!CurrentEquippedItem)
	{
		return;
	}

	// 무기인지 확인
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

// ===== 슬롯 전환 시스템 =====

void ADXPlayerCharacter::EquipSlot1()
{
	SwitchToSlot(1);
}

void ADXPlayerCharacter::EquipSlot2()
{
	SwitchToSlot(2);
}

void ADXPlayerCharacter::EquipSlot3()
{
	SwitchToSlot(3);
}

//==========================================
// 2. 플레이어가 슬롯 전환
//==========================================

void ADXPlayerCharacter::SwitchToSlot(int32 SlotNumber)
{
	if (!Inventory || SlotNumber < 1 || SlotNumber > 3)
	{
		return;
	}

	// 현재 같은 슬롯이면 무시
	if (CurrentSlot == SlotNumber && CurrentEquippedItem)
	{
		return;
	}

	// 현재 아이템 해제
	if (CurrentEquippedItem)
	{
		Inventory->UnequipCurrentItem();
	}

	// 새 슬롯의 아이템 장착
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

//==========================================
// 3. 아이템 드롭 시스템
//==========================================

void ADXPlayerCharacter::DropItemFromInventory(UItemBase* ItemToDrop, int32 QuantityToDrop)
{
	if (!ItemToDrop || !Inventory)
	{
		return;
	}

	// 서버에서 실행
	if (!HasAuthority())
	{
		// ServerDropItem RPC 호출
		return;
	}

	// 인벤토리에서 제거
	if (QuantityToDrop >= ItemToDrop->Quantity)
	{
		Inventory->RemoveItem(ItemToDrop);
	}
	else
	{
		Inventory->RemoveItemByQuantity(ItemToDrop, QuantityToDrop);
	}

	// 월드에 APickUp 스폰
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

		// 물리 활성화 (던지기 효과)
		if (DroppedPickup->PickUpMesh)
		{
			DroppedPickup->PickUpMesh->SetSimulatePhysics(true);
			DroppedPickup->PickUpMesh->AddImpulse(GetActorForwardVector() * 500.0f);
		}
	}
}

//==========================================
// 7. 특정 아이템 찾기
//==========================================

bool ADXPlayerCharacter::HasRequiredTool(FName ToolID)
{
	if (!Inventory)
	{
		return false;
	}

	UItemBase* Tool = Inventory->FindItemByID(ToolID);
	return Tool != nullptr;
}

void ADXPlayerCharacter::DropCurrentItem()
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

	// 아이템 분리
	CurrentEquippedItem->Detach();

	// 앞으로 던지기
	FVector ThrowDirection = GetActorForwardVector();
	FVector ThrowLocation = GetActorLocation() + (ThrowDirection * 100.0f);
	CurrentEquippedItem->SetActorLocation(ThrowLocation);

	// 물리 활성화 (던지기 효과)
	if (CurrentEquippedItem->ItemMesh)
	{
		CurrentEquippedItem->ItemMesh->SetSimulatePhysics(true);
		CurrentEquippedItem->ItemMesh->AddImpulse(ThrowDirection * 500.0f, NAME_None, true);
	}

	UE_LOG(LogTemp, Log, TEXT("[Player] Dropped item: %s"), *CurrentEquippedItem->GetName());

	// 참조 제거
	CurrentEquippedItem = nullptr;
	CurrentSlot = 0;

	// TODO: 인벤토리에서도 아이템 제거
	// Inventory->RemoveItem(ItemReference);

}

void ADXPlayerCharacter::UseEquippedItem()
{
	if (!CurrentEquippedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] No item equipped"));
		return;
	}

	// 아이템이 사용 가능한지 확인
	if (!CurrentEquippedItem->CanUse())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player] Item cannot be used"));
		return;
	}

	// 통합 방식: 모든 아이템은 Use() 함수를 가지고 있음
	CurrentEquippedItem->Use();

	UE_LOG(LogTemp, Log, TEXT("[Player] Used item: %s"), *CurrentEquippedItem->GetName());

	// ===== 개별 타입 체크 방식 (선택사항) =====
	// 특정 타입별로 다른 처리가 필요한 경우에만 사용

	// 도구 (Tools)
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
	// 무기 (Weapons)
	else if (AEquippableWeapon_* Weapon = Cast<AEquippableWeapon_>(CurrentEquippedItem))
	{
		Weapon->Attack();  // 무기는 공격
	}
	// 유틸리티 (Utilities)
	else if (AEquippableUtility* Utility = Cast<AEquippableUtility>(CurrentEquippedItem))
	{
		Utility->UseUtility();
	}
	// 기본 처리
	else
	{
		CurrentEquippedItem->Use();
	}
}


void ADXPlayerCharacter::StopUsingEquippedItem()
{
	if (!CurrentEquippedItem)
	{
		return;
	}

	CurrentEquippedItem->StopUsing();

	UE_LOG(LogTemp, Log, TEXT("[Player] Stopped using item: %s"), *CurrentEquippedItem->GetName());
}	

void ADXPlayerCharacter::PlayMeleeMontage(UAnimMontage* Montage)
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

void ADXPlayerCharacter::PlayToolUseMontage(UAnimMontage* Montage)
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

UItemBase* ADXPlayerCharacter::CreateItemFromDataTable(FName ItemID, int32 Quantity)
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

	// 데이터 복사
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

void ADXPlayerCharacter::GiveStartingItems()
{
	if (!HasAuthority() || !Inventory)
	{
		return;
	}

	// 렌치 지급
	UItemBase* Wrench = CreateItemFromDataTable(FName("Wrench"), 1);
	if (Wrench)
	{
		Inventory->HandleAddItem(Wrench);
		UE_LOG(LogTemp, Log, TEXT("[Player] Starting items given"));
	}
}

//==========================================
// REPLICATION CALLBACKS
//==========================================

void ADXPlayerCharacter::OnRep_CurrentEquippedItem()
{
	UE_LOG(LogTemp, Log, TEXT("[Player Client] CurrentEquippedItem updated"));

	// TODO: 클라이언트에서 아이템 표시 업데이트
	// 예: 3인칭에서 다른 플레이어가 무기를 바꿨을 때 보이도록
}

void ADXPlayerCharacter::OnRep_Health()
{
	UE_LOG(LogTemp, Log, TEXT("[Player Client] Health updated: %.1f"), Health);

	// UI 업데이트
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

//==========================================
// HEALTH SYSTEM
//==========================================

void ADXPlayerCharacter::SetHealth(float NewHealth)
{
	if (!HasAuthority())
	{
		return;
	}

	Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

	// 델리게이트 브로드캐스트
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.0f)
	{
		Die();
	}

	UE_LOG(LogTemp, Log, TEXT("[Player] Health set to: %.1f / %.1f"), Health, MaxHealth);
}

void ADXPlayerCharacter::Die()
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Player] Died"));

	// 사망 처리
	// 1. 캐릭터 비활성화
	GetCharacterMovement()->DisableMovement();

	// 2. 충돌 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. 현재 장착 아이템 드롭
	if (CurrentEquippedItem)
	{
		DropCurrentItem();
	}

	// 4. 인벤토리 아이템 모두 드롭 (선택사항)
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

	// 5. TODO: 사망 애니메이션 재생
	// 6. TODO: 리스폰 타이머 시작

	// 델리게이트 브로드캐스트
	OnHealthChanged.Broadcast(0.0f, MaxHealth);
}
// ========================================
// SERVER RPC IMPLEMENTATIONS
// ========================================

void ADXPlayerCharacter::ServerEquipItem_Implementation(AEquippableItem* Item)
{
	EquipItem(Item);
}

bool ADXPlayerCharacter::ServerEquipItem_Validate(AEquippableItem* Item)
{
	return Item != nullptr;
}

// ---

void ADXPlayerCharacter::ServerUnequipItem_Implementation()
{
	UnequipCurrentItem();
}

bool ADXPlayerCharacter::ServerUnequipItem_Validate()
{
	return true;
}

// ---

void ADXPlayerCharacter::ServerDropItem_Implementation()
{
	DropCurrentItem();
}

bool ADXPlayerCharacter::ServerDropItem_Validate()
{
	return true;
}

// ---
void ADXPlayerCharacter::ServerDropCurrentItem_Implementation()
{
	if (CurrentEquippedItem && Inventory)
	{
		// 현재 장착된 아이템의 ItemReference 찾기
		UItemBase* ItemToDrop = CurrentEquippedItem->ItemReference;
		if (ItemToDrop)
		{
			DropItemFromInventory(ItemToDrop, 1);
		}
	}
}

void ADXPlayerCharacter::ServerUseItem_Implementation()
{
	UseEquippedItem();
}

bool ADXPlayerCharacter::ServerUseItem_Validate()
{
	return CurrentEquippedItem != nullptr;
}

// ---

void ADXPlayerCharacter::ServerStopUsingItem_Implementation()
{
	StopUsingEquippedItem();
}

bool ADXPlayerCharacter::ServerStopUsingItem_Validate()
{
	return true;
}
