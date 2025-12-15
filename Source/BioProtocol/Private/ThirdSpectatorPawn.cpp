// Fill out your copyright notice in the Description page of Project Settings.


#include "ThirdSpectatorPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h" 
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h" 
#include <EnhancedInputComponent.h>
#include "InputActionValue.h" 
#include <EnhancedInputSubsystems.h>

// Sets default values

AThirdSpectatorPawn::AThirdSpectatorPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;

    CameraBoom->bUsePawnControlRotation = true;
   
    CameraBoom->bDoCollisionTest = false;

   
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false; 
}

void AThirdSpectatorPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AThirdSpectatorPawn, CurrentTarget);
}
// Called when the game starts or when spawned
void AThirdSpectatorPawn::BeginPlay()
{
	Super::BeginPlay();
    
   /* if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
                FString::Printf(TEXT("2")));
            Subsystem->AddMappingContext(InputMappingContext, 0);
        }
    }   */
}

// Called every frame
void AThirdSpectatorPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentTarget && IsValid(CurrentTarget))
    {
        FVector TargetLoc = CurrentTarget->GetActorLocation();
            
        TargetLoc.Z += 80.0f; 

        SetActorLocation(TargetLoc);
    }   

    if (IsLocallyControlled()) {
      
    }

}

// Called to bind functionality to input
void AThirdSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EIC && LookAction)
    {
        EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLookInput);
    }

    // 2. [추가] 여기서 Mapping Context를 등록해야 합니다.
    // PlayerInputComponent가 넘어왔다는 것은 Controller가 있다는 뜻입니다.
    APlayerController* PC = Cast<APlayerController>(GetController());

    // 만약 GetController()가 여전히 불안하다면, PlayerInputComponent의 Owner를 확인할 수도 있지만
    // 일반적으로 SetupPlayerInputComponent 시점엔 Controller가 유효합니다.
    if (PC)
    {
        if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                // 중복 방지를 위해 Clear 후 추가하거나, 안전하게 추가
                Subsystem->ClearAllMappings(); // 기존(이전 캐릭터) 매핑이 남아있을 수 있으므로 초기화 추천
                Subsystem->AddMappingContext(InputMappingContext, 0);
            }
        }
    }
}

void AThirdSpectatorPawn::HandleLookInput(const FInputActionValue& InValue)
{
    FVector2D LookAxisVector = InValue.Get<FVector2D>();

    if (Controller)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            FString::Printf(TEXT("input")));
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AThirdSpectatorPawn::SetSpectateTarget(AActor* NewTarget)
{
    if (HasAuthority()) 
    {
        CurrentTarget = NewTarget;
    }
}

void AThirdSpectatorPawn::SpectateNextPlayer()
{
    if (!HasAuthority())
    {
       
        return;
    }

    for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
    {
        ACharacter* OtherChar = *It;
        if (OtherChar && OtherChar != CurrentTarget && IsValid(OtherChar))
        {
            SetSpectateTarget(OtherChar);
            break; 
        }
    }
}

