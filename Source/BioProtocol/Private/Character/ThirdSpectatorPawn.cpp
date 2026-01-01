// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ThirdSpectatorPawn.h"
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
    CameraBoom->TargetArmLength = 200.0f;

    CameraBoom->bUsePawnControlRotation = true;   

    CameraBoom->bDoCollisionTest = true;

    CameraBoom->ProbeSize = 12.f;

    CameraBoom->ProbeChannel = ECC_Camera;

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
    APlayerController* PC = Cast<APlayerController>(GetController());
      
    if (PC)
    {
        if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                Subsystem->ClearAllMappings(); 
                Subsystem->AddMappingContext(InputMappingContext, 0);
            }
        }
    }

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

}

// Called to bind functionality to input
void AThirdSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EIC)
    {
        EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLookInput);
        EIC->BindAction(NextAction, ETriggerEvent::Triggered, this, &ThisClass::SpectateNextPlayer);
    }

    
}

void AThirdSpectatorPawn::HandleLookInput(const FInputActionValue& InValue)
{
    FVector2D LookAxisVector = InValue.Get<FVector2D>();

    if (Controller)
    {
      //  GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,FString::Printf(TEXT("input")));
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

void AThirdSpectatorPawn::Server_SpectateNextPlayer_Implementation()
{
    for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
    {
        ACharacter* OtherChar = *It;
        if (IsValid(OtherChar) && OtherChar != CurrentTarget)
        {
            CurrentTarget = OtherChar;

            Client_SetSpectateTarget(OtherChar);
            break;
        }
    }
}

void AThirdSpectatorPawn::Client_SetSpectateTarget_Implementation(ACharacter* NewTarget)
{
    SetSpectateTarget(NewTarget);

}

void AThirdSpectatorPawn::SpectateNextPlayer()
{ 
    if (!HasAuthority())
    {
        Server_SpectateNextPlayer();

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

