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
#include "Components/ChildActorComponent.h"
#include "Game/BioPlayerState.h"
#include "Daeho/MyInteractableInterface.h"
#include <Character/AndroidCharacter.h>
#include "Daeho/DH_PickupItem.h"
#include "Sound/SoundBase.h"

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
	GetMesh()->SetOwnerNoSee(true);

	SetItemMesh();

	// ������ �ɼ�
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(44.f);

	Status = CreateDefaultSubobject<UStaffStatusComponent>(TEXT("StatusComponent"));

	//CurrentTool = EToolType::Wrench;
}

// Called when the game starts or when spawned
void AStaffCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (ULocalPlayer* LP = PC->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* EILPS =
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
				{
					if (InputMappingContext)
					{
						EILPS->AddMappingContext(InputMappingContext, 0);
						UE_LOG(LogTemp, Log, TEXT("[Player LOCAL] Input mapping added"));
					}
				}
			}
		}
	}

	// 3. 서버 전용 초기화
	if (HasAuthority())
	{
		if (Status)
		{
			Status->ApplyBaseStatus();
			UE_LOG(LogTemp, Log, TEXT("[Player SERVER] Status initialized"));
		}
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRenderCustomDepth(true);

		MeshComp->SetCustomDepthStencilValue(1);
	}

	UpdateCharacterColor();
}

// Called every frame
void AStaffCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//if (IsLocallyControlled()) {
	//	GEngine->AddOnScreenDebugMessage(
	//		-1, 3.f, FColor::Yellow,
	//		FString::Printf(TEXT("Multicast | Local=%d"), bIsGunEquipped));
	//}

	//FVector Start = FirstPersonCamera->GetComponentLocation();
	//FVector End = Start + (FirstPersonCamera->GetForwardVector() * 250.0f); // 250cm 거리

	//FHitResult HitResult;
	//FCollisionQueryParams Params;
	//Params.AddIgnoredActor(this); // 내 몸은 무시

	//bool bHit = GetWorld()->LineTraceSingleByChannel(
	//	HitResult,
	//	Start,
	//	End,
	//	ECC_Visibility, // 보이는 물체 대상
	//	Params
	//);

	// 디버그 라인 그리기 (개발 확인용)

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
	EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &ThisClass::AttackInput);

	EIC->BindAction(TestKillAction, ETriggerEvent::Started, this, &ThisClass::TestHit);

	EIC->BindAction(TestPullLever, ETriggerEvent::Started, this, &ThisClass::InteractPressed);
	EIC->BindAction(DropAction, ETriggerEvent::Started, this, &ThisClass::KOnDrop);

	EIC->BindAction(Item1, ETriggerEvent::Started, this, &ThisClass::EquipSlot1);
	EIC->BindAction(Item2, ETriggerEvent::Started, this, &ThisClass::EquipSlot2);
	EIC->BindAction(Item3, ETriggerEvent::Started, this, &ThisClass::EquipSlot3);

}

void AStaffCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AStaffCharacter, MaterialIndex);
	DOREPLIFETIME(AStaffCharacter, bIsCanAttack);

	DOREPLIFETIME(AStaffCharacter, bHasTorch);
	DOREPLIFETIME(AStaffCharacter, bHasWrench);
	DOREPLIFETIME(AStaffCharacter, bHasGun);
	DOREPLIFETIME(AStaffCharacter, bHasPotion);

	DOREPLIFETIME(AStaffCharacter, bIsGunEquipped);
	DOREPLIFETIME(AStaffCharacter, bIsPotionEquipped);
	DOREPLIFETIME(AStaffCharacter, bIsTorchEquipped);
	DOREPLIFETIME(AStaffCharacter, bIsWrenchEquipped);
	DOREPLIFETIME(AStaffCharacter, InventoryDurability);
	DOREPLIFETIME(AStaffCharacter, Ammo);
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
	if (Status->CurrentStamina <= 0.f || !Status->bIsRunable)
	{
		GetCharacterMovement()->MaxWalkSpeed = Status->MoveSpeed;
		ServerStopRun();
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed =
		Status->MoveSpeed * Status->RunMultiply;

	ServerStartRun();
}
void AStaffCharacter::HandleStopRun(const FInputActionValue& InValue)
{
	GetCharacterMovement()->MaxWalkSpeed = Status->MoveSpeed;

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
	//UseEquippedItem();

	if (CurrentTool == EToolType::None&&!bIsGunEquipped) {
		if (!bIsCanAttack) return;
		ServerRPCMeleeAttack();
		return;
	}
	if (Ammo > 0 && CurrentTool == EToolType::Gun) {
		if (!bCanFire) return;

		bCanFire = false;

		if (IsLocallyControlled())
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				GunFireSound,
				GetActorLocation()
			);
		}
		Server_GunHit();
		GetWorldTimerManager().SetTimer(
			FireCooldownHandle,
			this,
			&AStaffCharacter::ResetFire,
			FireInterval,
			false
		);
	}
	else if (Ammo <= 0 && CurrentTool == EToolType::Gun) {
		if (IsLocallyControlled())
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				GunEmptySound,
				GetActorLocation()
			);
		}
	}

	if (bHasPotion && CurrentTool == EToolType::Potion) {
		ServerUsePotion();
		UnequipAll();//손 템 치우기
		bHasPotion = false;
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

void AStaffCharacter::TryEquipPotion()
{
	if (!bHasPotion)
		return;
	if (PotionMesh) {
		UnequipAll();

		PotionMesh->SetVisibility(!bIsPotionEquipped, true);
		PotionMesh->SetHiddenInGame(bIsPotionEquipped);
	}

	bIsPotionEquipped = !bIsPotionEquipped;
	if (bIsPotionEquipped)
		CurrentTool = EToolType::Potion;
	else {
		CurrentTool = EToolType::None;
	}
	if (!HasAuthority())
	{
		ServerEquipPotion();
		return;
	}
}

void AStaffCharacter::TryEquipGun()
{
	if (!bHasGun)
		return;
	if (WeaponMesh)
	{
		UnequipAll();

		WeaponMesh->SetVisibility(!bIsGunEquipped, true);
		WeaponMesh->SetHiddenInGame(bIsGunEquipped);
	}
	bIsGunEquipped = !bIsGunEquipped;
	if (bIsGunEquipped) {
		CurrentTool = EToolType::Gun;
	}
	else {
		CurrentTool = EToolType::None;
	}

	if (!HasAuthority())
	{
		ServerEquipGun();
		return;
	}

	//OnRep_GunEquipped();
}

void AStaffCharacter::TryEquipTorch()
{
	if (!bHasTorch)
		return;
	if (TorchMesh)
	{
		UnequipAll();

		TorchMesh->SetVisibility(!bIsTorchEquipped, true);
		TorchMesh->SetHiddenInGame(bIsTorchEquipped);
	}
	bIsGunEquipped = false;

	bIsTorchEquipped = !bIsTorchEquipped;
	if (bIsTorchEquipped) {
		CurrentTool = EToolType::Welder;
	}
	else {
		CurrentTool = EToolType::None;
	}
	if (!HasAuthority())
	{
		ServerEquipTorch();
		return;
	}


	//OnRep_TorchEquipped();
}

void AStaffCharacter::TryEquipWrench()
{
	if (!bHasWrench)
		return;
	if (WrenchMesh)
	{
		UnequipAll();
		WrenchMesh->SetVisibility(!bIsWrenchEquipped, true);
		WrenchMesh->SetHiddenInGame(bIsWrenchEquipped);
	}

	bIsGunEquipped = false;

	bIsWrenchEquipped = !bIsWrenchEquipped;
	if (bIsWrenchEquipped) {
		CurrentTool = EToolType::Wrench;
	}
	else {
		CurrentTool = EToolType::None;
	}
	if (!HasAuthority())
	{
		ServerEquipWrench();
		return;
	}


	//	OnRep_WrenchEquipped();
}

void AStaffCharacter::ServerUsePotion_Implementation()
{
	Status->SetFullCurrentHP();
	bIsPotionEquipped = false;
	bHasPotion = false;
}

void AStaffCharacter::ServerEquipPotion_Implementation()
{
	bIsPotionEquipped = !bIsPotionEquipped;
	UnequipAll();
	bIsGunEquipped = false;
	bIsTorchEquipped = false;
	bIsWrenchEquipped = false;


}

void AStaffCharacter::ServerEquipGun_Implementation()
{
	bIsGunEquipped = !bIsGunEquipped;
	UnequipAll();
	bIsPotionEquipped = false;
	bIsTorchEquipped = false;
	bIsWrenchEquipped = false;

	//bIsGunEquipped = !bIsGunEquipped;

	if (bIsGunEquipped) {
		CurrentTool = EToolType::Gun;
	}
	else {
		CurrentTool = EToolType::None;
	}

	OnRep_TorchEquipped();
	OnRep_WrenchEquipped();
	OnRep_GunEquipped();
}

void AStaffCharacter::ServerEquipTorch_Implementation()
{
	bIsTorchEquipped = !bIsTorchEquipped;
	UnequipAll();
	bIsGunEquipped = false;
	bIsPotionEquipped = false;

	if (bIsTorchEquipped) {
		CurrentTool = EToolType::Welder;
	}
	else {
		CurrentTool = EToolType::None;
	}
	//bIsGunEquipped = false;
	//bIsWrenchEquipped = false;

	//bIsTorchEquipped = !bIsTorchEquipped;

	OnRep_GunEquipped();
	OnRep_WrenchEquipped();
	OnRep_TorchEquipped();
}

void AStaffCharacter::ServerEquipWrench_Implementation()
{
	bIsWrenchEquipped = !bIsWrenchEquipped;
	UnequipAll();
	bIsGunEquipped = false;
	bIsPotionEquipped = false;

	if (bIsWrenchEquipped) {
		CurrentTool = EToolType::Wrench;
	}
	else {
		CurrentTool = EToolType::None;
	}

	//bIsWrenchEquipped = !bIsWrenchEquipped;

	OnRep_GunEquipped();
	OnRep_TorchEquipped();
	OnRep_WrenchEquipped();
}

void AStaffCharacter::ReleaseLever()
{
	if (!IsLocallyControlled())
		return;

	bHoldingLever = false;

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
			FPAnim->Montage_Play(MeleeAttackMontageFP);
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (IsValid(AnimInstance) == true)
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void AStaffCharacter::MissionInteract()
{
	//UE_LOG(LogTemp, Log, TEXT("0"));

	ServerMissionInteract();
}

void AStaffCharacter::ItemInteract()
{
	ServerItemInteract();
}

void AStaffCharacter::ServerItemInteract_Implementation()
{
	FVector Start;
	FRotator ControlRot;

	Controller->GetPlayerViewPoint(Start, ControlRot);
	// 또는 GetActorEyesViewPoint(Start, ControlRot);

	FVector End = Start + (ControlRot.Vector() * 250.f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		Params
	);

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

void AStaffCharacter::ServerMissionInteract_Implementation()
{
	//UE_LOG(LogTemp, Log, TEXT("1"));

	FVector Start;
	FRotator ControlRot;

	Controller->GetPlayerViewPoint(Start, ControlRot);
	// 또는 GetActorEyesViewPoint(Start, ControlRot);

	FVector End = Start + (ControlRot.Vector() * 250.f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	if (bHit && HitResult.GetActor() &&
		HitResult.GetActor()->GetClass()->ImplementsInterface(UMyInteractableInterface::StaticClass()))
	{
		IMyInteractableInterface::Execute_Interact(HitResult.GetActor(), this);
	}
}

float AStaffCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority()) return 0.f;

	const float ActualDamage =
		Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (AStaffCharacter* Victim = Cast<AStaffCharacter>(this))
	{
		Victim->Client_PlayHitSound();
	}

	if (ActualDamage <= 0.f) return 0.f;

	if (Status)
	{
		Status->ApplyDamage(ActualDamage, EventInstigator);
		if (Status->IsDead()) {
			Client_OnToolBroken();
			bHasGun = false;
			Ammo = 0;
			bHasPotion = false;
			bHasWrench = false;
			bHasTorch = false;
			bIsGunEquipped = false;
			bIsWrenchEquipped = false;
			bIsTorchEquipped = false;
			bIsPotionEquipped = false;
		}
	}

	return ActualDamage;
}

void AStaffCharacter::TestHit()
{
	Server_Hit();
}

void AStaffCharacter::Client_PlayHitSound_Implementation()
{
	if (TakeDamageSound)
	{
		UGameplayStatics::PlaySound2D(
			this,
			TakeDamageSound
		);
	}
}

void AStaffCharacter::Multicast_PlayGunSound_Implementation()
{	
	if (!IsLocallyControlled())
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			GunFireSound,
			GetActorLocation(),
			1.f,
			1.f,
			0.f,
			GunShotAttenuation
		);
	}
}

void AStaffCharacter::Server_GunHit_Implementation()
{
	float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < FireInterval)
		return;

	LastFireTime = Now;

	FVector Start;
	FRotator ControlRot;
	--Ammo;
	Controller->GetPlayerViewPoint(Start, ControlRot);

	Multicast_PlayGunSound();

	FVector End = Start + (ControlRot.Vector() * 3000.f);

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
		30,
		GetController(),
		this,
		UDamageType::StaticClass()
	);
}

void AStaffCharacter::ServerSetCanAttack_Implementation(bool bCanAttack)
{
	bIsCanAttack = !bIsCanAttack;
}

void AStaffCharacter::OnRep_GunEquipped()
{
	if (ThirdWeaponMesh)
	{
		ThirdWeaponMesh->SetVisibility(bIsGunEquipped, true);
		ThirdWeaponMesh->SetHiddenInGame(!bIsGunEquipped);
	}

	if (bIsGunEquipped) {
		CurrentTool = EToolType::Gun;
	}
	else if(CurrentTool!=EToolType::Potion){
		CurrentTool = EToolType::None;
	}
}

void AStaffCharacter::OnRep_WrenchEquipped()
{
	if (ThirdWrenchMesh)
	{
		ThirdWrenchMesh->SetVisibility(bIsWrenchEquipped, true);
		ThirdWrenchMesh->SetHiddenInGame(!bIsWrenchEquipped);
	}

	if (bIsWrenchEquipped) {
		CurrentTool = EToolType::Wrench;
	}
	else if(CurrentTool!=EToolType::Gun&& CurrentTool != EToolType::Potion){
		//fuck

		CurrentTool = EToolType::None;
	}
}

void AStaffCharacter::OnRep_TorchEquipped()
{
	if (ThirdTorchMesh)
	{
		ThirdTorchMesh->SetVisibility(bIsTorchEquipped, true);
		ThirdTorchMesh->SetHiddenInGame(!bIsTorchEquipped);
	}
	if (bIsTorchEquipped) {
		CurrentTool = EToolType::Welder;
	}
	else if (CurrentTool != EToolType::Gun && CurrentTool != EToolType::Potion) {
		CurrentTool = EToolType::None;

	}

}

void AStaffCharacter::OnRep_PotionEquipped()
{
	if (ThirdPotionMesh)
	{
		ThirdPotionMesh->SetVisibility(bIsPotionEquipped, true);
		ThirdPotionMesh->SetHiddenInGame(!bIsPotionEquipped);
	}
	if (bIsTorchEquipped) {
		CurrentTool = EToolType::Welder;
	}
	else if (CurrentTool != EToolType::Gun) {
		CurrentTool = EToolType::None;

	}
}

void AStaffCharacter::OnRep_BIsCanAttack()
{

}


void AStaffCharacter::OnRep_MaterialIndex()
{
	/*if (IsLocallyControlled())
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::FromInt(MaterialIndex));*/
	if (mat.IsValidIndex(MaterialIndex))
	{
		GetMesh()->SetMaterial(0, mat[MaterialIndex]);
	}
}


void AStaffCharacter::Server_Hit_Implementation()
{

	FVector Start;
	FRotator ControlRot;

	Controller->GetPlayerViewPoint(Start, ControlRot);

	FVector End = Start + (ControlRot.Vector() * 250.f);

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

	float DamageAmount = 10.f; // 기본 데미지

	if (AAndroidCharacter* Android = Cast<AAndroidCharacter>(this))
	{
		if (Android->bIsHunter)
		{
			DamageAmount = 50.f;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Hit %s"), *HitActor->GetName());

	UGameplayStatics::ApplyDamage(
		HitActor,
		DamageAmount,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

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

void AStaffCharacter::EquipSlot1(const FInputActionValue& InValue)
{
	TryEquipGun();
}

void AStaffCharacter::EquipSlot2(const FInputActionValue& InValue)
{	

	if (!bHasWrench && !bHasTorch) {
		return;
	}
	if (bHasWrench)
		TryEquipWrench();
	else if (bHasTorch)
		TryEquipTorch();
}

void AStaffCharacter::EquipSlot3(const FInputActionValue& InValue)
{
	TryEquipPotion();
}

void AStaffCharacter::InteractPressed(const FInputActionValue& InValue)
{
	ItemInteract();
}


void AStaffCharacter::Die(AController* KillerController)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* KillerPawn = KillerController->GetPawn();

	if (AAndroidCharacter* Killer = Cast<AAndroidCharacter>(KillerPawn))
	{
		Killer->bHasKilledPlayer = true;
	}
	
}

void AStaffCharacter::OnRep_bHasTorch()
{

}

void AStaffCharacter::OnRep_bHasWrench()
{

}

void AStaffCharacter::OnRep_bHasGun()
{
}

void AStaffCharacter::UnequipAll()
{
	UnequipGun();
	UnequipTorch();
	UnequipWrench();
	UnequipPotion();
}

void AStaffCharacter::UnequipGun()
{
	WeaponMesh->SetHiddenInGame(true);
	WeaponMesh->SetVisibility(false, true);
	WeaponMesh->SetCastShadow(false);
	//bIsGunEquipped = false;
}

void AStaffCharacter::UnequipTorch()
{
	TorchMesh->SetHiddenInGame(true);
	TorchMesh->SetVisibility(false, true);
	TorchMesh->SetCastShadow(false);

	//bIsWrenchEquipped = false;
}

void AStaffCharacter::UnequipWrench()
{
	WrenchMesh->SetHiddenInGame(true);
	WrenchMesh->SetVisibility(false, true);
	WrenchMesh->SetCastShadow(false);
}

void AStaffCharacter::UnequipPotion()
{
	PotionMesh->SetHiddenInGame(true);
	PotionMesh->SetVisibility(false, true);
	PotionMesh->SetCastShadow(false);
}

bool AStaffCharacter::KServerPickUpItem(EToolType NewItemType, int32 NewDurability)
{
	switch (NewItemType)
	{
	case EToolType::None:
		return 0;
	case EToolType::Wrench:
		if (bHasWrench || bHasTorch)
			return 0;
		bHasWrench = true;
		InventoryDurability = NewDurability;
		return 1;

		break;
	case EToolType::Welder:
		if (bHasWrench || bHasTorch)
			return 0;
		bHasTorch = true;
		InventoryDurability = NewDurability;

		return 1;
		break;
	case EToolType::Gun:
		bHasGun = true;
		Ammo += NewDurability;
		UE_LOG(LogTemp, Log, TEXT("now ammo: %d"), Ammo);

		return 1;
	case EToolType::Potion:
		if (bHasPotion)
			return 0;
		bHasPotion = true;
		return 1;

	default:
		return 0;
		break;
	}
}

void AStaffCharacter::KConsumeToolDurability(int32 Amount)
{
	if (CurrentTool == EToolType::None) return;


	InventoryDurability -= Amount;


	UE_LOG(LogTemp, Log, TEXT("Tool Used. Remaining Durability: %d"), InventoryDurability);

	if (CurrentTool == EToolType::Gun && Ammo <= 0)
	{
		Ammo = 0;

	}
	else if (InventoryDurability <= 0)
	{
		// 내구도 0 -> 아이템 파괴
		switch (CurrentTool)
		{
		case EToolType::Wrench:
			bHasWrench = false;
			break;
		case EToolType::Welder:
			bHasTorch = false;
			break;
		default:
			break;
		}

		// 맨손으로 전환
			// 상태만 변경
		bIsTorchEquipped = false;
		//bIsGunEquipped = false;
		bIsWrenchEquipped = false;
		bIsPotionEquipped = false;

		// 서버는 직접 호출
		OnRep_TorchEquipped();
		OnRep_GunEquipped();
		OnRep_WrenchEquipped();
		OnRep_PotionEquipped();

		InventoryDurability = 0;
		Client_OnToolBroken();

		UE_LOG(LogTemp, Warning, TEXT("Tool Broken!"));
	}

}

void AStaffCharacter::KOnDrop()
{
	if (CurrentTool == EToolType::None) return;

	UnequipAll();
	KServerDropItem();
}

void AStaffCharacter::KServerDropItem_Implementation()
{
	if (CurrentTool == EToolType::None) return;
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
			if (CurrentTool != EToolType::Gun) {
				// 버리는 아이템의 정보 그대로 전달
				DroppedItem->InitializeDrop(CurrentTool, InventoryDurability);
			}
			else {
				DroppedItem->InitializeDrop(CurrentTool, Ammo);
			}

			// 스폰 완료
			UGameplayStatics::FinishSpawningActor(DroppedItem, FTransform(SpawnRot, SpawnLoc));
		}
	}

	switch (CurrentTool)
	{
	case EToolType::Wrench:
		bHasWrench = false;
		break;
	case EToolType::Welder:
		bHasTorch = false;
		break;
	case EToolType::Gun: {
		bHasGun = false;
		Ammo = 0;
	}
	case EToolType::Potion: {
		bHasPotion = false;
		//Ammo = 0;
	}
	default:
		break;
	}

	// 맨손으로 전환
		// 상태만 변경
	bIsTorchEquipped = false;
	bIsGunEquipped = false;
	bIsWrenchEquipped = false;
	bIsPotionEquipped = false;
	//bIsGunEquipped = false;

	// 서버는 직접 호출
	OnRep_TorchEquipped();
	OnRep_GunEquipped();
	OnRep_WrenchEquipped();
	OnRep_PotionEquipped();

	CurrentTool = EToolType::None;
	InventoryDurability = 0;

	UE_LOG(LogTemp, Log, TEXT("Item Dropped."));
}

void AStaffCharacter::Client_OnToolBroken_Implementation()
{
	UnequipAll();
}

void AStaffCharacter::ServerCleanHands_Implementation()
{
	// 맨손으로 전환
		// 상태만 변경
	bIsTorchEquipped = false;
	bIsGunEquipped = false;
	bIsWrenchEquipped = false;
	bIsPotionEquipped = false;

	// 서버는 직접 호출
	OnRep_TorchEquipped();
	OnRep_GunEquipped();
	OnRep_WrenchEquipped();
	OnRep_PotionEquipped();

	CurrentTool = EToolType::None;
	//IventoryDurability = 0;

}
void AStaffCharacter::SetItemMesh()
{
	////////////////////////////////////////////////////////////////////////////////
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(FirstPersonMesh, TEXT("hand_r"));
	WeaponMesh->SetHiddenInGame(true);
	WeaponMesh->SetVisibility(false, true);
	WeaponMesh->SetCastShadow(false);

	// 3��Ī �޽��� ���ο��� �� ���̰�
	ThirdWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdWeaponMesh"));
	ThirdWeaponMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	ThirdWeaponMesh->SetHiddenInGame(true);
	ThirdWeaponMesh->SetVisibility(false, true);
	ThirdWeaponMesh->SetCastShadow(false);
	ThirdWeaponMesh->SetOwnerNoSee(true);

	TorchMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TorchMesh"));
	TorchMesh->SetupAttachment(FirstPersonMesh, TEXT("hand_r"));
	TorchMesh->SetHiddenInGame(true);
	TorchMesh->SetVisibility(false, true);
	TorchMesh->SetCastShadow(false);

	ThirdTorchMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdTorchMesh"));
	ThirdTorchMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	ThirdTorchMesh->SetHiddenInGame(true);
	ThirdTorchMesh->SetVisibility(false, true);
	ThirdTorchMesh->SetCastShadow(false);
	ThirdTorchMesh->SetOwnerNoSee(true);

	WrenchMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WrenchMesh"));
	WrenchMesh->SetupAttachment(FirstPersonMesh, TEXT("hand_r"));
	WrenchMesh->SetHiddenInGame(true);
	WrenchMesh->SetVisibility(false, true);
	WrenchMesh->SetCastShadow(false);

	ThirdWrenchMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdWrenchMesh"));
	ThirdWrenchMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	ThirdWrenchMesh->SetHiddenInGame(true);
	ThirdWrenchMesh->SetVisibility(false, true);
	ThirdWrenchMesh->SetCastShadow(false);
	ThirdWrenchMesh->SetOwnerNoSee(true);

	PotionMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PotionMesh"));
	PotionMesh->SetupAttachment(FirstPersonMesh, TEXT("hand_r"));
	PotionMesh->SetHiddenInGame(true);
	PotionMesh->SetVisibility(false, true);
	PotionMesh->SetCastShadow(false);

	ThirdPotionMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPotionMesh"));
	ThirdPotionMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	ThirdPotionMesh->SetHiddenInGame(true);
	ThirdPotionMesh->SetVisibility(false, true);
	ThirdPotionMesh->SetCastShadow(false);
	ThirdPotionMesh->SetOwnerNoSee(true);

}

void AStaffCharacter::ResetFire()
{
	bCanFire = true;
}

void AStaffCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	UpdateCharacterColor();
}

void AStaffCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UpdateCharacterColor();
}

void AStaffCharacter::UpdateCharacterColor()
{
	ABioPlayerState* BioPS = GetPlayerState<ABioPlayerState>();
	if (!BioPS) return;

	BioPS->OnColorChanged.RemoveDynamic(this, &AStaffCharacter::OnColorIndexChanged);
	BioPS->OnColorChanged.AddDynamic(this, &AStaffCharacter::OnColorIndexChanged);

	OnColorIndexChanged(BioPS->ColorIndex);
}

void AStaffCharacter::OnColorIndexChanged(int32 NewIndex)
{
	if (ColorSettings.IsValidIndex(NewIndex))
	{
		const FStaffColorInfo& SelectedColor = ColorSettings[NewIndex];

		if (GetMesh())
		{
			if (SelectedColor.Material_01)
			{
				GetMesh()->SetMaterial(0, SelectedColor.Material_01);
			}

			if (SelectedColor.Material_02)
			{
				GetMesh()->SetMaterial(1, SelectedColor.Material_02);
			}

			UE_LOG(LogTemp, Log, TEXT("[StaffCharacter] Color Applied: Index %d"), NewIndex);
		}
	}
}