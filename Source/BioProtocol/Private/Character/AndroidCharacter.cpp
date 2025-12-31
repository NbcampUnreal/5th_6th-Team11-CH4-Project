#include "Character/AndroidCharacter.h"
#include <EnhancedInputComponent.h>
#include "Engine/Engine.h"
#include "Components/PostProcessComponent.h"
#include <EnhancedInputSubsystems.h>
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include <Game/BioGameMode.h>
#include "Game/BioGameState.h"
#include "Game/BioProtocolTypes.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"

AAndroidCharacter::AAndroidCharacter()
{
	bReplicates = true;

	//SwitchToAndroidMode();
	PostProcessComp = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComp"));
	PostProcessComp->SetupAttachment(RootComponent);

	PostProcessComp->bUnbound = true;
	PostProcessComp->Priority = 10.0f;

	AndroidFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AndroidFX"));

	AndroidFX->SetupAttachment(GetMesh(), TEXT("Eye_L_Socket"));
	AndroidFX->SetRelativeLocation(FVector::ZeroVector);
	AndroidFX->SetRelativeRotation(FRotator::ZeroRotator);

	AndroidFX->bAutoActivate = false;

	AndroidFX2 = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AndroidFX2"));

	AndroidFX2->SetupAttachment(GetMesh(), TEXT("Eye_R_Socket"));
	AndroidFX2->SetRelativeLocation(FVector::ZeroVector);
	AndroidFX2->SetRelativeRotation(FRotator::ZeroRotator);
	AndroidFX2->bAutoActivate = false;

	BreathAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("BreathAudio"));
	BreathAudio->SetupAttachment(RootComponent);

	BreathAudio->bAutoActivate = false;
	BreathAudio->bIsUISound = false;
}

void AAndroidCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* Mat = XRayMaterial.LoadSynchronous())
	{
		FWeightedBlendable Blendable;
		Blendable.Object = Mat;
		Blendable.Weight = 1.f;

		PostProcessComp->Settings.WeightedBlendables.Array.Add(Blendable);
	}
	PostProcessComp->bEnabled = bIsXray;

	if (IsLocallyControlled() == true)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		checkf(IsValid(PC) == true, TEXT("PlayerController is invalid."));

		UEnhancedInputLocalPlayerSubsystem* EILPS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
		checkf(IsValid(EILPS) == true, TEXT("EnhancedInputLocalPlayerSubsystem is invalid."));

		EILPS->AddMappingContext(SkillMappingContext, 1);
	}
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	BaseCapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
	BaseCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	BaseMeshOffset = GetMesh()->GetRelativeLocation();
	BaseMeshScale = GetMesh()->GetRelativeScale3D();
	BaseCameraOffset = FirstPersonCamera->GetRelativeLocation();

	if (AndroidBreathSound)
	{
		BreathAudio->SetSound(AndroidBreathSound);
	}

	if (BreathAtt)
	{
		BreathAudio->AttenuationSettings = BreathAtt;
	}

}

void AAndroidCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EIC->BindAction(ChangeAction, ETriggerEvent::Started, this, &ThisClass::SwitchAndroidMode);

	EIC->BindAction(XrayAction, ETriggerEvent::Started, this, &ThisClass::Xray);
	EIC->BindAction(DashAction, ETriggerEvent::Started, this, &ThisClass::OnDash);
	EIC->BindAction(ScaleChangeAction, ETriggerEvent::Started, this, &ThisClass::Server_OnChangeMode);

}

void AAndroidCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAndroidCharacter, bIsAndroid);
	DOREPLIFETIME(AAndroidCharacter, CharacterScale);
	DOREPLIFETIME(AAndroidCharacter, bIsHunter);
	DOREPLIFETIME(AAndroidCharacter, bHasKilledPlayer);
}


void AAndroidCharacter::UpdateBreathSound()
{
	if (bIsHunter)
	{
		if (!BreathAudio->IsPlaying())
		{
			BreathAudio->Play();
		}
	}
	else
	{
		if (BreathAudio->IsPlaying())
		{
			BreathAudio->FadeOut(0.3f, 0.f);
		}
	}
}

void AAndroidCharacter::OnDash()
{
	if (!bIsHunter) return;

	if (HasAuthority())
		Server_Dash();
	else
		Server_Dash();
}

void AAndroidCharacter::OnChangeMode(float scale)
{
	UnequipAll();

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	UCharacterMovementComponent* Move = GetCharacterMovement();

	const float NewRadius = BaseCapsuleRadius * scale;
	const float NewHalfHeight = BaseCapsuleHalfHeight * scale;

	Capsule->SetCapsuleSize(NewRadius, NewHalfHeight, true);

	GetMesh()->SetRelativeScale3D(BaseMeshScale * scale);

	const float DeltaHalfHeight = NewHalfHeight - BaseCapsuleHalfHeight;

	FVector NewMeshLocation = BaseMeshOffset;
	NewMeshLocation.Z -= DeltaHalfHeight;

	GetMesh()->SetRelativeLocation(NewMeshLocation);

	if (IsLocallyControlled())
	{
		FirstPersonCamera->SetRelativeLocation(BaseCameraOffset * scale);
	}

	/*Move->MaxStepHeight = BaseMaxStepHeight * scale;
	Move->BrakingDecelerationWalking = BaseBrakingDecel * scale;*/
}

void AAndroidCharacter::OnRep_CharacterScale()
{
	OnChangeMode(CharacterScale);
	UpdateEyeFX(bIsHunter);
	UnequipAll();
}

void AAndroidCharacter::Server_OnChangeMode_Implementation()
{
	/*if (!IsNightPhase())
		return;*/

	if (HasAuthority())
	{
		if (!bIsHunter) {
			CharacterScale = HunterScale;
			ServerCleanHands();
		}
		else
			CharacterScale = NormalScale;

		OnRep_CharacterScale();
		bIsHunter = !bIsHunter;
	}
}

void AAndroidCharacter::Server_Dash_Implementation()
{
	//UE_LOG(LogTemp, Warning, TEXT("(dash)"));
	FVector Start = GetActorLocation();
	FVector Forward = GetActorForwardVector();
	FVector End = Start + Forward * BlinkDistance;

	FCollisionShape Capsule = FCollisionShape::MakeCapsule(
		GetCapsuleComponent()->GetScaledCapsuleRadius(),
		GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
	);

	FHitResult Hit;
	bool bBlocked = GetWorld()->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Capsule,
		FCollisionQueryParams(TEXT("BlinkSweep"), false, this)
	);

	FVector FinalLocation = bBlocked ? Hit.Location : End;

	TeleportTo(FinalLocation, GetActorRotation(), false, true);
}

void AAndroidCharacter::Xray()
{
	if (!PostProcessComp || !bIsHunter) return;

	PostProcessComp->bEnabled = !PostProcessComp->bEnabled;

	bIsXray = PostProcessComp->bEnabled;
}

bool AAndroidCharacter::IsNightPhase()
{
	if (const ABioGameState* GS = GetWorld()->GetGameState<ABioGameState>())
	{
		return GS->CurrentPhase == EBioGamePhase::Night;
	}
	return false;
}

void AAndroidCharacter::AndroidArmAttack()
{


}

void AAndroidCharacter::AttackInput(const FInputActionValue& InValue)
{
	if (bIsHunter && bHasKilledPlayer) {
		return;
	}
	else if (bIsHunter && !bHasKilledPlayer) {
		ServerRPCMeleeAttack();
		return;
	}
	Super::AttackInput(1);

}

void AAndroidCharacter::ServerPullLever_Internal()
{
	if (bIsHunter)
		return;

	Super::ServerPullLever_Internal();
}

void AAndroidCharacter::PullLever()
{
	if (bIsHunter)
		return;

	Super::PullLever();
}

void AAndroidCharacter::OnRep_IsHunter()
{
	UpdateBreathSound();
}

void AAndroidCharacter::OnRep_IsAndroid()
{
	if (bIsAndroid)
	{
		GetMesh()->SetSkeletalMesh(AndroidMesh);
		GetMesh()->SetAnimInstanceClass(AndroidAnim);
		MeleeAttackMontage = AndroidMeleeAttackMontage;
	}
	else
	{
		GetMesh()->SetSkeletalMesh(StaffMesh);
		GetMesh()->SetAnimInstanceClass(StaffAnim);
		MeleeAttackMontage = OriginMeleeAttackMontage;
	}

	// --- 1인칭: 자기 자신만 ---
	if (IsLocallyControlled())
	{
		if (bIsAndroid)
		{
			FirstPersonMesh->SetSkeletalMesh(AndroidArmMesh);
			FirstPersonMesh->SetAnimInstanceClass(AndroidAnim);

			//  FirstPersonMesh->SetRelativeLocation(FVector(0.f, 1.f, 0.f));

		}
		else
		{
			FirstPersonMesh->SetSkeletalMesh(StaffArmMesh);
			FirstPersonMesh->SetAnimInstanceClass(StaffAnim);

			//FirstPersonMesh->SetRelativeLocation(FVector(0.f, -1.f, 0.f));

		}
	}
}

void AAndroidCharacter::UpdateEyeFX(int8 val)
{
	if (IsLocallyControlled())
	{
		AndroidFX->Deactivate();
		AndroidFX2->Deactivate();

		return;
	}

	if (val) {
		AndroidFX->Activate(true);
		AndroidFX2->Activate(true);

	}
	else {
		AndroidFX->Deactivate();
		AndroidFX2->Deactivate();

	}
}

void AAndroidCharacter::EquipSlot1(const FInputActionValue& InValue)
{
	if (!bIsHunter) {
		Super::EquipSlot1(1);
	}
}

void AAndroidCharacter::EquipSlot2(const FInputActionValue& InValue)
{
	if (!bIsHunter) {
		Super::EquipSlot2(2);
	}
}

void AAndroidCharacter::EquipSlot3(const FInputActionValue& InValue)
{
	if (!bIsHunter) {
		Super::EquipSlot3(3);
	}
}

void AAndroidCharacter::InteractPressed(const FInputActionValue& InValue)
{
	if (!bIsHunter) {
		Super::InteractPressed(3);
	}
}

void AAndroidCharacter::ServerSwitchToStaff_Implementation()
{
}

void AAndroidCharacter::ServerSwitchAndroid_Implementation()
{
	SwitchAndroidMode();
}

void AAndroidCharacter::SwitchAndroidMode()
{

	bool bMode = !bIsAndroid;

	// --- 서버가 아닌 경우 RPC 호출 ---
	if (!HasAuthority())
	{
		ServerSwitchAndroid();
	}

	// ---------- 1인칭 ----------
	if (IsLocallyControlled())
	{
		if (bMode)
		{
			FirstPersonMesh->SetSkeletalMesh(AndroidArmMesh);
			FirstPersonMesh->SetAnimInstanceClass(AndroidAnim);
		}
		else
		{
			FirstPersonMesh->SetSkeletalMesh(StaffArmMesh);
			FirstPersonMesh->SetAnimInstanceClass(StaffAnim);
		}
	}

	// ---------- 3인칭 ----------
	if (bMode)
	{
		GetMesh()->SetSkeletalMesh(AndroidMesh);
		GetMesh()->SetAnimInstanceClass(AndroidAnim);
		MeleeAttackMontage = AndroidMeleeAttackMontage;
	}
	else
	{
		GetMesh()->SetSkeletalMesh(StaffMesh);
		GetMesh()->SetAnimInstanceClass(StaffAnim);
		MeleeAttackMontage = OriginMeleeAttackMontage;
	}

	// ---------- 서버만 Replicate ----------
	if (HasAuthority())
	{
		bIsAndroid = bMode;
		// OnRep 자동 실행됨(서버→클라)
	}
}
