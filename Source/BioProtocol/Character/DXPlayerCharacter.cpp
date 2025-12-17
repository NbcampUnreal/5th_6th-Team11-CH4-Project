#include "DXPlayerCharacter.h"

#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"


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
		&ACharacter::StopJumping
	);
}

void ADXPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocallyControlled() == true)
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

	const FVector Forward = GetActorForwardVector();
	FVector TraceStart{ GetPawnViewLocation()};
	FVector TraceEnd{ TraceStart + (GetViewRotation().Vector() * InteractionCheckDistance) };

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

	if (IsValid(TargetInteractable.GetObject()))
	{
		TargetInteractable->Interact();
	}
}
