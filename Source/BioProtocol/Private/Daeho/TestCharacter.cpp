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
#include "Net/UnrealNetwork.h"

// Sets default values
ATestCharacter::ATestCharacter()
{
    // Tick 활성화 (필요 없다면 false로 해도 되지만 기본적으로 둡니다)
    PrimaryActorTick.bCanEverTick = true;

    // 멀티플레이어에서 이 캐릭터가 네트워크로 복제되도록 설정
    bReplicates = true;

    // 1. 캡슐 컴포넌트 크기 설정 (기본값)
    GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

    // 2. 1인칭 카메라 생성 및 설정
    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent()); // 캡슐에 부착
    FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // 눈 높이 설정
    FirstPersonCameraComponent->bUsePawnControlRotation = true; // 컨트롤러 회전(마우스)을 따름

    // 3. 캐릭터 무브먼트 설정 (멀티플레이어 동기화는 기본적으로 CharacterMovementComponent가 처리함)
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

    // 1. 주인(나)에게는 이 메쉬를 렌더링하지 않습니다.
    GetMesh()->SetOwnerNoSee(true);
    // 2. 하지만 그림자는 주인에게도 보이게 합니다 (몸은 투명해도 그림자는 있어야 리얼함).
    GetMesh()->bCastHiddenShadow = true;

    // 기본 도구는 맨손
    CurrentTool = EToolType::None;
}

void ATestCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // CurrentTool 변수가 변경되면 클라이언트에 복제됨
    DOREPLIFETIME(ATestCharacter, CurrentTool);
}

void ATestCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Enhanced Input 시스템에 매핑 컨텍스트 추가 (로컬 플레이어인 경우에만)
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            // 컨텍스트가 유효하다면 시스템에 추가
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

    // 기존 InputComponent를 EnhancedInputComponent로 캐스팅
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // 점프 바인딩
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }

        // 이동 바인딩
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATestCharacter::Move);
        }

        // 시선 처리 바인딩
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATestCharacter::Look);
        }

		// 상호작용 바인딩
        if (InteractAction)
        {
            EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ATestCharacter::Interact);
        }

        // 도구 변경 (1, 2, 3 키)
        if (EquipSlot1Action)
        {
            EnhancedInputComponent->BindAction(EquipSlot1Action, ETriggerEvent::Started, this, &ATestCharacter::OnEquipSlot1);
        }
        if (EquipSlot2Action)
        {
            EnhancedInputComponent->BindAction(EquipSlot2Action, ETriggerEvent::Started, this, &ATestCharacter::OnEquipSlot2);
        }
        if (EquipSlot3Action)
        {
            EnhancedInputComponent->BindAction(EquipSlot3Action, ETriggerEvent::Started, this, &ATestCharacter::OnEquipSlot3);
        }
    }
}

void ATestCharacter::Move(const FInputActionValue& Value)
{
    // 입력값 가져오기 (Vector2D: X=전후, Y=좌우)
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 카메라가 바라보는 방향을 기준으로 이동
        AddMovementInput(GetActorForwardVector(), MovementVector.Y); // 전후 (Y축이 Forward/Backward)
        AddMovementInput(GetActorRightVector(), MovementVector.X);   // 좌우 (X축이 Right/Left)
    }
}

void ATestCharacter::Look(const FInputActionValue& Value)
{
    // 입력값 가져오기 (Vector2D: X=Yaw, Y=Pitch)
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 마우스 X 이동 -> 캐릭터 회전 (Yaw)
        AddControllerYawInput(LookAxisVector.X);
        // 마우스 Y 이동 -> 카메라 상하 (Pitch)
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

// --- 도구 변경 로직 ---

void ATestCharacter::OnEquipSlot1(const FInputActionValue& Value)
{
    // 1번: 맨손
    ServerEquipTool(EToolType::None);
}

void ATestCharacter::OnEquipSlot2(const FInputActionValue& Value)
{
    // 2번: 렌치
    ServerEquipTool(EToolType::Wrench);
}

void ATestCharacter::OnEquipSlot3(const FInputActionValue& Value)
{
    // 3번: 용접기
    ServerEquipTool(EToolType::Welder);
}

void ATestCharacter::ServerEquipTool_Implementation(EToolType NewTool)
{
    // 서버에서 변수 변경 -> Replicated되어 클라에도 전파됨
    CurrentTool = NewTool;

    // 로그로 확인
    const UEnum* ToolEnum = StaticEnum<EToolType>();
    UE_LOG(LogTemp, Log, TEXT("Equipped Tool: %s"), *ToolEnum->GetNameStringByValue((int64)CurrentTool));
}

void ATestCharacter::Interact(const FInputActionValue& Value)
{
    // 클라이언트에서 키를 누르면 서버에게 처리 요청
    ServerInteract();
}

void ATestCharacter::ServerInteract_Implementation()
{
    // 서버에서 실행되는 로직: 카메라 앞쪽으로 레이저 발사
    FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 250.0f); // 250cm 거리

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 내 몸은 무시

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_Visibility, // 보이는 물체 대상
        Params
    );

    // 디버그 라인 그리기 (개발 확인용)
    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f);

    if (bHit && HitResult.GetActor())
    {
        // 맞은 물체가 인터페이스를 구현하고 있는지 확인
        if (HitResult.GetActor()->GetClass()->ImplementsInterface(UMyInteractableInterface::StaticClass()))
        {
            // 인터페이스 함수 호출 (IInteractableInterface::Execute_함수이름)
            IMyInteractableInterface::Execute_Interact(HitResult.GetActor(), this);
        }
    }
}
