#include "Daeho/TestCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Daeho/MyInteractableInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Daeho/DH_PickupItem.h"

// Sets default values
ATestCharacter::ATestCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
    FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
    FirstPersonCameraComponent->bUsePawnControlRotation = true;

    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->bCastHiddenShadow = true;

    bIsToolEquipped = false;
    InventoryItemType = EToolType::None;
    InventoryDurability = 0;
}

void ATestCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATestCharacter, bIsToolEquipped);
    DOREPLIFETIME(ATestCharacter, InventoryItemType);
    DOREPLIFETIME(ATestCharacter, InventoryDurability);
}

void ATestCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Enhanced Input ?쒖뒪?쒖뿉 留ㅽ븨 而⑦뀓?ㅽ듃 異붽? (濡쒖뺄 ?뚮젅?댁뼱??寃쎌슦?먮쭔)
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            // 而⑦뀓?ㅽ듃媛 ?좏슚?섎떎硫??쒖뒪?쒖뿉 異붽?
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
}

void ATestCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ATestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // 湲곗〈 InputComponent瑜?EnhancedInputComponent濡?罹먯뒪??
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // ?먰봽 諛붿씤??
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }

        // ?대룞 諛붿씤??
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATestCharacter::Move);
        }

        // ?쒖꽑 泥섎━ 諛붿씤??
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATestCharacter::Look);
        }

		// ?곹샇?묒슜 諛붿씤??
        if (InteractAction)
        {
            EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ATestCharacter::Interact);
        }

        // ?꾧뎄 蹂寃?(1, 2, 3 ??
        if (Slot1Action)
        {
            EnhancedInputComponent->BindAction(Slot1Action, ETriggerEvent::Started, this, &ATestCharacter::OnSlot1);
        }
        if (Slot2Action)
        {
            EnhancedInputComponent->BindAction(Slot2Action, ETriggerEvent::Started, this, &ATestCharacter::OnSlot2);
        }
        if (DropAction)
        {
            EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &ATestCharacter::OnDrop);
        }
    }
}

void ATestCharacter::Move(const FInputActionValue& Value)
{
    // ?낅젰媛?媛?몄삤湲?(Vector2D: X=?꾪썑, Y=醫뚯슦)
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 移대찓?쇨? 諛붾씪蹂대뒗 諛⑺뼢??湲곗??쇰줈 ?대룞
        AddMovementInput(GetActorForwardVector(), MovementVector.Y); // ?꾪썑 (Y異뺤씠 Forward/Backward)
        AddMovementInput(GetActorRightVector(), MovementVector.X);   // 醫뚯슦 (X異뺤씠 Right/Left)
    }
}

void ATestCharacter::Look(const FInputActionValue& Value)
{
    // ?낅젰媛?媛?몄삤湲?(Vector2D: X=Yaw, Y=Pitch)
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 留덉슦??X ?대룞 -> 罹먮┃???뚯쟾 (Yaw)
        AddControllerYawInput(LookAxisVector.X);
        // 留덉슦??Y ?대룞 -> 移대찓???곹븯 (Pitch)
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

// --- ?꾧뎄 蹂寃?濡쒖쭅 ---

EToolType ATestCharacter::GetCurrentActiveTool() const
{
    // 도구 장착 모드이고, 인벤토리에 아이템이 있어야만 도구로 인정
    if (bIsToolEquipped && InventoryItemType != EToolType::None)
    {
        return InventoryItemType;
    }
    return EToolType::None; // 그 외엔 맨손
}

void ATestCharacter::OnSlot1(const FInputActionValue& Value)
{
    ServerSetEquipState(false); // 맨손 모드
}

void ATestCharacter::OnSlot2(const FInputActionValue& Value)
{
    ServerSetEquipState(true); // 도구 장착 모드
}

void ATestCharacter::ServerSetEquipState_Implementation(bool bEquip)
{
    bIsToolEquipped = bEquip;

    // 로그 출력
    EToolType Current = GetCurrentActiveTool();
    const UEnum* EnumPtr = StaticEnum<EToolType>();
    UE_LOG(LogTemp, Log, TEXT("Current Tool State: %s"), *EnumPtr->GetNameStringByValue((int64)Current));
}

bool ATestCharacter::ServerPickUpItem(EToolType NewItemType, int32 NewDurability)
{
    // 이미 인벤토리에 아이템이 있으면 못 줍습니다.
    if (InventoryItemType != EToolType::None)
    {
        return false;
    }

    // 아이템 획득
    InventoryItemType = NewItemType;
    InventoryDurability = NewDurability;

    // 자동으로 도구를 든 상태로 전환해줍니다.
    bIsToolEquipped = true;

    UE_LOG(LogTemp, Log, TEXT("Picked Up: %d (Dur: %d)"), (int32)NewItemType, NewDurability);
    return true;
}

void ATestCharacter::OnDrop(const FInputActionValue& Value)
{
    ServerDropItem();
}

void ATestCharacter::ServerDropItem_Implementation()
{
    // 인벤토리가 비었으면 아무것도 안 함
    if (InventoryItemType == EToolType::None) return;

    // 스폰 위치: 캐릭터 앞 100cm
    FVector SpawnLoc = GetActorLocation() + (GetActorForwardVector() * 100.0f);
    FRotator SpawnRot = GetActorRotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 아이템 액터 스폰 (Deferred Spawn 사용: 스폰 전에 변수 설정을 위해)
    if (PickupItemClass)
    {
        ADH_PickupItem* DroppedItem = GetWorld()->SpawnActorDeferred<ADH_PickupItem>(
            PickupItemClass,
            FTransform(SpawnRot, SpawnLoc),
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
        );

        if (DroppedItem)
        {
            // 버리는 아이템의 정보 그대로 전달
            DroppedItem->InitializeDrop(InventoryItemType, InventoryDurability);

            // 스폰 완료
            UGameplayStatics::FinishSpawningActor(DroppedItem, FTransform(SpawnRot, SpawnLoc));
        }
    }

    // 인벤토리 비우기
    InventoryItemType = EToolType::None;
    InventoryDurability = 0;
    bIsToolEquipped = false; // 맨손으로 전환

    UE_LOG(LogTemp, Log, TEXT("Item Dropped."));
}

void ATestCharacter::ConsumeToolDurability(int32 Amount)
{
    // 맨손이면 내구도 없음
    if (InventoryItemType == EToolType::None) return;

    InventoryDurability -= Amount;

    UE_LOG(LogTemp, Log, TEXT("Tool Used. Remaining Durability: %d"), InventoryDurability);

    if (InventoryDurability <= 0)
    {
        // 내구도 0 -> 아이템 파괴
        InventoryItemType = EToolType::None;
        InventoryDurability = 0;
        bIsToolEquipped = false;

        UE_LOG(LogTemp, Warning, TEXT("Tool Broken!"));
    }
}

void ATestCharacter::Interact(const FInputActionValue& Value)
{
    ServerInteract();
}

void ATestCharacter::ServerInteract_Implementation()
{
    // ?쒕쾭?먯꽌 ?ㅽ뻾?섎뒗 濡쒖쭅: 移대찓???욎そ?쇰줈 ?덉씠? 諛쒖궗
    FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 250.0f); // 250cm 嫄곕━

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // ??紐몄? 臾댁떆

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_Visibility, // 蹂댁씠??臾쇱껜 ???
        Params
    );

    // ?붾쾭洹??쇱씤 洹몃━湲?(媛쒕컻 ?뺤씤??
    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f);

    if (bHit && HitResult.GetActor())
    {
        if (HitResult.GetActor()->IsA(ADH_PickupItem::StaticClass()))
        {
            // 인터페이스 호출 -> PickupItem::Interact 실행 -> Character::ServerPickUpItem 실행
            if (HitResult.GetActor()->GetClass()->ImplementsInterface(UMyInteractableInterface::StaticClass()))
            {
                IMyInteractableInterface::Execute_Interact(HitResult.GetActor(), this);
            }
        }
        // 2. TaskObject인 경우 -> 임무 수행
        else if (HitResult.GetActor()->GetClass()->ImplementsInterface(UMyInteractableInterface::StaticClass()))
        {
            IMyInteractableInterface::Execute_Interact(HitResult.GetActor(), this);
        }
    }
}
