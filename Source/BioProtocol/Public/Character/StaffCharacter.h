// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Daeho/TestCharacter.h"
#include "StaffCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UMaterialInterface;
class IInteractionInterface;
class UChildActorComponent;
class USoundBase;
class USoundAttenuation;
class UStaticMeshComponent;
//class EToolType; //테스트

USTRUCT()
struct FInteractionData
{
	GENERATED_USTRUCT_BODY()

	FInteractionData() : CurrentInteractable(nullptr), LastInteractionCheckTime(0.f) {}

	UPROPERTY()
	AActor* CurrentInteractable;

	UPROPERTY()
	float LastInteractionCheckTime;
};

USTRUCT(BlueprintType)
struct FStaffColorInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UMaterialInterface* Material_01;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UMaterialInterface* Material_02;
};

UCLASS()
class BIOPROTOCOL_API AStaffCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AStaffCharacter();

	virtual void OnRep_PlayerState() override;
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	TArray<FStaffColorInfo> ColorSettings;

	UFUNCTION(BlueprintCallable)
	void UpdateCharacterColor();

	UFUNCTION()
	void OnColorIndexChanged(int32 NewIndex);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void OnDeath();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetMaterialByIndex(int32 NewIndex);

public:
	bool IsGunEquipped() const { return bIsGunEquipped; };
	void PlayMeleeAttackMontage(UAnimMontage* Montage);
	void MissionInteract();
	void ItemInteract();
	UFUNCTION(Server, Reliable)
	void ServerMissionInteract();
	UFUNCTION(Server, Reliable)
	void ServerItemInteract();
	EToolType GetCurrentTool() { return CurrentTool; };

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRPCMeleeAttack();

	virtual void AttackInput(const FInputActionValue& InValue);

	//�׽�Ʈ�� �Լ�(���� �ǰ��׽�Ʈ(Server_TestHit()))
	void TestHit();

	UFUNCTION(Server, Reliable)
	void Server_Hit();
	UFUNCTION(Server, Reliable)
	void Server_GunHit();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetTestMaterial();

	UFUNCTION(NetMulticast, Reliable)

	void Multicast_PlayGunSound();
	UFUNCTION(Client, Reliable)
	void Client_PlayHitSound();

	UFUNCTION(Server, Reliable)
	void ServerSetCanAttack(bool bCanAttack);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Gun")
	USoundBase* GunFireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Gun")
	USoundBase* GunEmptySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Gun")
	USoundBase* TakeDamageSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundAttenuation* GunShotAttenuation;
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundAttenuation* FootStepAttenuation;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tool")
	EToolType CurrentTool;//테스트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FirstPersonCamera;
	

private:
	void HandleMoveInput(const FInputActionValue& InValue);

	void HandleLookInput(const FInputActionValue& InValue);

	//�޸������(����Ʈ ����)
	void HandleStartRun(const FInputActionValue& InValue);

	//�޸������(����Ʈ ����)
	void HandleStopRun(const FInputActionValue& InValue);

	void HandleCrouch(const FInputActionValue& InValue);

	void HandleStand(const FInputActionValue& InValue);



protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext; //�⺻ ������ , ��ȣ�ۿ�

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> MoveAction;//wasd

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> LookAction;//���콺

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> JumpAction;//�����̽���
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> RunAction;//����Ʈ

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> AttackAction;//Ŭ��

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> TestKillAction;//k (�Լ�ȣ�� �׽�Ʈ��)

	//������ �Դ�Ű�����µ� �̰� ���������� �������� �˻��� ������ PullLever ȣ���ϴ� ������� �ٲ�� ����
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> TestPullLever;

	//item�̳� inventoryŬ�����ȿ� ������ ������ �Է� ���������� ����
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> Item1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> Item2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> Item3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> TestItem2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> TestItem3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* DropAction;

protected:
	//�������� ����(�ƴϸ� ��ȣ�ۿ�����ϸ鼭 �þ߰� �����ʿ� �Ҷ� ȣ��)
	virtual void PullLever();

	//���� ����
	void ReleaseLever();

	UFUNCTION(Server, Reliable)
	void ServerPullLever();
	UFUNCTION(Server, Reliable)
	//////////////////�������� ��ȣ�ۿ� �׽�Ʈ��
	void ServerReleaseLever();

	//���� �������� ���� �۾��Ҳ� �۾��� �Լ�
	virtual void ServerPullLever_Internal();
	void TestUpdateLeverGauge(); //������ ���°� �׽�Ʈ��(���� �����ۿ��ϴ� ������Ʈ�� ����)
	float TestGuage = 0;//�׽�Ʈ������



	FTimerHandle GaugeTimerHandle;

private:
	uint8 bIsRunning = false;
	uint8 bHoldingLever = false;
	float LeverBaseYaw = 0.f;

	UFUNCTION(Server, Reliable)
	void ServerStartRun();

	UFUNCTION(Server, Reliable)
	void ServerStopRun();

	UFUNCTION(Server, Reliable)
	void ServerCrouch();

	UFUNCTION(Server, Reliable)
	void ServerUnCrouch();

	void OnJump(const FInputActionValue& InValue);
	void OnStopJump(const FInputActionValue& InValue);

	UFUNCTION(Server, Reliable)
	void ServerOnJump();

	UFUNCTION(Server, Reliable)
	void ServerStopJump();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCMeleeAttack();


	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	//UFUNCTION(Server, Reliable, WithValidation)
	//void ServerRPCTakeDamage(float DamageAmount);

	UFUNCTION()
	void OnRep_BIsCanAttack();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//ü��,���׹̳�,�̵��ӵ� ����Ǿ��ִ� Ŭ����
	TObjectPtr<class UStaffStatusComponent> Status;

	UFUNCTION()
	void OnRep_MaterialIndex();

	UPROPERTY(ReplicatedUsing = OnRep_MaterialIndex)
	int32 MaterialIndex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> MeleeAttackMontageFP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* StaffMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* StaffArmMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMat")
	//�÷��̾� ���п� ���׸���
	TArray<UMaterialInterface*>mat;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_BIsCanAttack)
	bool bIsCanAttack = true;
	///////////////////////////////////////////�κ�,�����۰���
public:
	void Die(AController* KillerController);

	UFUNCTION(BlueprintCallable, Category = "Slots")
	virtual void EquipSlot1(const FInputActionValue& InValue);

	UFUNCTION(BlueprintCallable, Category = "Slots")
	virtual void EquipSlot2(const FInputActionValue& InValue);

	UFUNCTION(BlueprintCallable, Category = "Slots")
	virtual void EquipSlot3(const FInputActionValue& InValue);
		
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void InteractPressed(const FInputActionValue& InValue);

	//////////////////////////temp템
public:
	UFUNCTION()
	void OnRep_bHasTorch();
	UFUNCTION()
	void OnRep_bHasWrench();
	UFUNCTION()
	void OnRep_bHasGun();

	UFUNCTION()
	void OnRep_GunEquipped(); //다른클라에서 내 메쉬 on/off
	UFUNCTION()
	void OnRep_WrenchEquipped();
	UFUNCTION()
	void OnRep_TorchEquipped();
	UFUNCTION()
	void OnRep_PotionEquipped();


	void TryEquipPotion();
	void TryEquipGun();
	void TryEquipTorch();
	void TryEquipWrench();

	UFUNCTION(Server, Reliable)
	void ServerUsePotion();
	UFUNCTION(Server, Reliable)
	void ServerEquipPotion();
	UFUNCTION(Server, Reliable)
	void ServerEquipGun();
	UFUNCTION(Server, Reliable)
	void ServerEquipTorch();
	UFUNCTION(Server, Reliable)
	void ServerEquipWrench();

	void UnequipAll();
	void UnequipGun();
	void UnequipTorch();
	void UnequipWrench();
	void UnequipPotion();	

	bool KServerPickUpItem(EToolType NewItemType, int32 NewDurability);
	void KConsumeToolDurability(int32 Amount);
	void KOnDrop();
	UFUNCTION(Server, Reliable)
	void KServerDropItem();
	UFUNCTION(Server, Reliable)

	void ServerCleanHands();

	void SetItemMesh();

	UFUNCTION(Client, Reliable)
	void Client_OnToolBroken();

	UPROPERTY(EditDefaultsOnly, Category = "Gun")
	float FireInterval = 0.3f;

	bool bCanFire = true;
	FTimerHandle FireCooldownHandle;
	void ResetFire();
	float LastFireTime = 0.f;

protected:
	//템소지 여부 bool
	UPROPERTY(ReplicatedUsing = OnRep_bHasTorch, BlueprintReadOnly, Category = "Equipment")
	uint8 bHasTorch;	
	UPROPERTY(ReplicatedUsing = OnRep_bHasTorch, BlueprintReadOnly, Category = "Equipment")
	uint8 bHasPotion;
	UPROPERTY(ReplicatedUsing = OnRep_bHasWrench, BlueprintReadOnly, Category = "Equipment")
	uint8 bHasWrench;
	UPROPERTY(ReplicatedUsing = OnRep_bHasGun, BlueprintReadOnly, Category = "Equipment")
	uint8 bHasGun;
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<ADH_PickupItem> PickupItemClass;

	//지금 손에 가지고있냐 여부 bool 이 변수값 바뀌면 다른 플레이어가 내 캐릭 손에 템 쥐고있는거 봄(OnRep_GunEquipped)
	UPROPERTY(ReplicatedUsing = OnRep_GunEquipped)
	bool bIsGunEquipped = false;
	UPROPERTY(ReplicatedUsing = OnRep_TorchEquipped)
	bool bIsTorchEquipped = false;
	UPROPERTY(ReplicatedUsing = OnRep_WrenchEquipped)
	bool bIsWrenchEquipped = false;
	UPROPERTY(ReplicatedUsing = OnRep_PotionEquipped)
	bool bIsPotionEquipped = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	int32 InventoryDurability;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	int32 Ammo=0;
	/// <summary>
	/// 손에붙일 템메쉬 (1인칭/다른클라3인칭)
	/// </summary>
	//총
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdWeaponMesh;
	//토치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* TorchMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdTorchMesh;
	//렌치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WrenchMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdWrenchMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* PotionMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPotionMesh;
};
