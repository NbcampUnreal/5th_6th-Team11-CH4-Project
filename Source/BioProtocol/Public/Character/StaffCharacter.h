// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BioProtocol/Public/Inventory/InventoryComponent.h"
#include "Interfaces/InteractionInterface.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "StaffCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UMaterialInterface;
class UInventoryComponent;
class AEquippableItem;
class UItemBase;
class IInteractionInterface;

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

UCLASS()
class BIOPROTOCOL_API AStaffCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AStaffCharacter();

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

protected:

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

	void AttackInput(const FInputActionValue& InValue);

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
	TObjectPtr<UInputAction> TestItem1;

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

	void TestItemSlot1();

	UFUNCTION(Server, Reliable)
	void ServerTestItemSlot1();

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

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRPCMeleeAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCMeleeAttack();


	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	//UFUNCTION(Server, Reliable, WithValidation)
	//void ServerRPCTakeDamage(float DamageAmount);

	//�׽�Ʈ�� �Լ�(���� �ǰ��׽�Ʈ(Server_TestHit()))
	void TestHit();

	UFUNCTION(Server, Reliable)
	void Server_TestHit();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetTestMaterial();

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


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* StaffMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* StaffArmMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterMat")
	//�÷��̾� ���п� ���׸���
	TArray<UMaterialInterface*>mat;

protected:
	UPROPERTY(Replicated)
	bool bIsGunEquipped;

	///////////////////////////////////////////�κ�,�����۰���
public:
	void Die();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", Replicated)
	class UInventoryComponent* Inventory;  // <- �ϳ��� ����

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FORCEINLINE UInventoryComponent* GetInventory() const { return Inventory; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayToolUseMontage(UAnimMontage* Montage);
	//==========================================
	// EQUIPMENT
	//==========================================

	UPROPERTY(ReplicatedUsing = OnRep_CurrentEquippedItem, BlueprintReadOnly, Category = "Equipment")
	AEquippableItem* CurrentEquippedItem;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
	int32 CurrentSlot;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	AEquippableItem* GetCurrentEquippedItem() const { return CurrentEquippedItem; }

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EquipItem(AEquippableItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UnequipCurrentItem();

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void DropCurrentItem();

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UseEquippedItem();

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void StopUsingEquippedItem();

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ReloadWeapon();
	//==========================================
	// SLOTS
	//==========================================

	UFUNCTION(BlueprintCallable, Category = "Slots")
	void EquipSlot1(const FInputActionValue& InValue);

	UFUNCTION(BlueprintCallable, Category = "Slots")
	void EquipSlot2(const FInputActionValue& InValue);

	UFUNCTION(BlueprintCallable, Category = "Slots")
	void EquipSlot3(const FInputActionValue& InValue);

	UFUNCTION(BlueprintCallable, Category = "Slots")
	void SwitchToSlot(int32 SlotNumber);

	//==========================================
	// INTERACTION
	//==========================================

	/** ���� ��ȣ�ۿ� ������ Ȯ�� */
	FORCEINLINE bool IsInteracting() const
	{
		return GetWorldTimerManager().IsTimerActive(TimerHandle_Interaction);
	}

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	AActor* CurrentInteractable;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionRange;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckFrequency;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckDistance;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void InteractPressed(const FInputActionValue& InValue);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void InteractReleased(const FInputActionValue& InValue);

	//==========================================
	// ITEMS
	//==========================================

	UFUNCTION(BlueprintCallable, Category = "Items")
	void DropItemFromInventory(UItemBase* ItemToDrop, int32 QuantityToDrop);

	UFUNCTION(BlueprintCallable, Category = "Items")
	bool HasRequiredTool(FName ToolID);

	UFUNCTION(BlueprintCallable, Category = "Items")
	void GiveStartingItems();

	UPROPERTY(EditDefaultsOnly, Category = "Items")
	UDataTable* ItemDataTable;

	//==========================================
	// INTERACTION INTERNAL
	//==========================================

	TScriptInterface<IInteractionInterface> TargetInteractable;
	FTimerHandle TimerHandle_Interaction;
	FTimerHandle InteractionTimerHandle;
	FInteractionData InteractionData;

	void PerformInteractionCheck();
	void FoundInteractable(AActor* NewInteractable);
	void NoInteractableFound();
	void BeginInteract();
	void EndInteract();
	void Interact();

	//==========================================
	// HELPER FUNCTIONS
	//==========================================

	UItemBase* CreateItemFromDataTable(FName ItemID, int32 Quantity = 1);

	//==========================================
	// SERVER RPC
	//==========================================

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipItem(AEquippableItem* Item);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUnequipItem();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerDropItem();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerDropCurrentItem();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseItem();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopUsingItem();

	//==========================================
	// REPLICATION CALLBACKS
	//==========================================

	UFUNCTION()
	void OnRep_CurrentEquippedItem();
	void DropCurrentItemInput(const FInputActionValue& InValue);

//==========================================
// HEALTH
//==========================================
public:

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealth(float NewHealth) { Health = FMath::Clamp(NewHealth, 0.f, MaxHealth); }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Health = 100.f;

};
