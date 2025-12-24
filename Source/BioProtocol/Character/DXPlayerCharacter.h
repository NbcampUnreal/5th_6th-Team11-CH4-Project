#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BioProtocol/Public/Interfaces/InteractionInterface.h"
#include "DXPlayerCharacter.generated.h"

class UCameraComponent;
class UInventoryComponent;
class UItemBase;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class AEquippableItem;

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
class BIOPROTOCOL_API ADXPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADXPlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//==========================================
	// COMPONENTS
	//==========================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInventoryComponent* Inventory;  // <- 하나만 유지

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UInventoryComponent* GetInventoryComponent() const { return Inventory; }

	//==========================================
	// EQUIPMENT
	//==========================================

	UPROPERTY(ReplicatedUsing = OnRep_CurrentEquippedItem, BlueprintReadOnly, Category = "Equipment")
	AEquippableItem* CurrentEquippedItem;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
	int32 CurrentSlot;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	AEquippableItem* GetCurrentEquippedItem() const { return CurrentEquippedItem; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	UInventoryComponent* GetInventory() const { return Inventory; }  // <- 인라인 구현으로 간단하게

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
	// ANIMATION
	//==========================================

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayMeleeMontage(UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayToolUseMontage(UAnimMontage* Montage);

	//==========================================
	// SLOTS
	//==========================================

	UFUNCTION(BlueprintCallable, Category = "Slots")
	void EquipSlot1();

	UFUNCTION(BlueprintCallable, Category = "Slots")
	void EquipSlot2();

	UFUNCTION(BlueprintCallable, Category = "Slots")
	void EquipSlot3();

	UFUNCTION(BlueprintCallable, Category = "Slots")
	void SwitchToSlot(int32 SlotNumber);

	//==========================================
	// INTERACTION
	//==========================================

	/** 현재 상호작용 중인지 확인 */
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
	void InteractPressed();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void InteractReleased();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void UpdateInteractionWidget(const FInteractableData& Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void HideInteractionWidget();

	//==========================================
	// HEALTH
	//==========================================

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Health")
	float Health;  // <- CurrentHealth → Health로 통일

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }  // <- 인라인 구현

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }  // <- 인라인 구현

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealth(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Die();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChanged OnHealthChanged;

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

protected:
	//==========================================
	// INPUT
	//==========================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* UseItemAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* DropItemAction;

	void HandleMoveInput(const FInputActionValue& InValue);
	void HandleLookInput(const FInputActionValue& InValue);
	void HandleUseItem();
	void HandleDropItem();

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
	void CheckForInteractables();

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

	UFUNCTION(Server, Reliable)
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

	UFUNCTION()
	void OnRep_Health();
};