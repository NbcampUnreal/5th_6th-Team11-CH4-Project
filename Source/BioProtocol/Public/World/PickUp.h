#pragma once

#include "CoreMinimal.h"
#include "Interfaces/InteractionInterface.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "GameFramework/Actor.h"
#include "PickUp.generated.h"

class UItemBase;
class UDataTable;

UCLASS()
class BIOPROTOCOL_API APickUp : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
	
public:	

	APickUp();

	//게임 시작 시 데이터베이스에서 모든 정보 가져오기
	void InitializePickup(const TSubclassOf<UItemBase> Baseclass, const int32 ItemQuantity);

	//아이템 드롭 시 이미 존재하는 아이템 참조에서 모든 정보 가져오기
	void InitializeDrop(UItemBase* ItemToDrop, const int32 ItemQuantity);

	FORCEINLINE UItemBase* GetItemData() const { return ItemReference; };

	virtual void BeginFocus() override;
	virtual void EndFocus() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "PickUp | Components")
	UStaticMeshComponent* PickUpMesh;

	//아이템 아이디 기준 정렬
	UPROPERTY(EditInstanceOnly, Category = "PickUp | Item Database")
	UDataTable* ItemDataTable;

	UPROPERTY(EditInstanceOnly, Category = "PickUp | Item Reference")
	int32 ItemQuantity;

	UPROPERTY(VisibleAnywhere, Category = "PickUp | Item Reference")
	UItemBase* ItemReference;

	UPROPERTY(VisibleInstanceOnly, Category = "PickUp | Interaction")
	FInteractableData InstanceInteractableData;

	UPROPERTY(EditInstanceOnly, Category = "PickUp | Item Database")
	FName DesiredItemID;

	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Interact(ADXPlayerCharacter* PlayerCharacter);

	void UpdateInteractableData();
	
	void TakePickup(const ADXPlayerCharacter* Taker);
};
