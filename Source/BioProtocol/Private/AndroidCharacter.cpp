#include "AndroidCharacter.h"
#include <EnhancedInputComponent.h>
#include "Engine/Engine.h"
#include "Components/PostProcessComponent.h"
#include <EnhancedInputSubsystems.h>

AAndroidCharacter::AAndroidCharacter()
{
	bReplicates = true;

	//SwitchToAndroidMode();
    PostProcessComp = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComp"));
    PostProcessComp->SetupAttachment(RootComponent);
    
    PostProcessComp->bUnbound = true; 
    PostProcessComp->Priority = 10.0f; 

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
}
void AAndroidCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EIC->BindAction(ChangeAction, ETriggerEvent::Started, this, &ThisClass::SwitchAndroidMode);

    EIC->BindAction(XrayAction, ETriggerEvent::Started, this, &ThisClass::Xray);
}

void AAndroidCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAndroidCharacter, bIsAndroid);
}

void AAndroidCharacter::Xray()
{
    if (!PostProcessComp) return;
       
    PostProcessComp->bEnabled = !PostProcessComp->bEnabled;   

    bIsXray = PostProcessComp->bEnabled;
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

void AAndroidCharacter::SwitchToStaff()
{
}
