// ItemBase.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BioProtocol/Public/World/ItemDataStructs.h"  // ItemDataStructs.h Include
#include "ItemBase.generated.h"

/**
 * 인벤토리에서 사용되는 아이템 데이터 클래스
 * 실제 월드에 존재하지 않는 순수 데이터 오브젝트 (UObject)
 */
UCLASS()
class BIOPROTOCOL_API UItemBase : public UObject
{
	GENERATED_BODY()

public:
	UItemBase();

	//==========================================
	// REPLICATED PROPERTIES
	//==========================================

	/** 아이템 고유 ID */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemQuality ItemQuality;

	/** 아이템 희귀도 */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	EItemRarity Rarity;

	/** 아이템 카테고리 */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	EItemCategory Category;

	/** 현재 수량 */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	int32 Quantity;

	/** 숫자 데이터 (무게, 스택, 내구도 등) */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	FItemNumericData NumericData;

	/** 텍스트 데이터 (이름, 설명 등) */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	FItemTextData TextData;

	/** 에셋 데이터 (아이콘, 메시) */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	FItemAssetData AssetData;

	// 장착 가능한 액터 클래스 
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	TSubclassOf<AEquippableItem> ItemClass;
	

	/** 효과 RowID (FItemEffectData 참조) */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	FName EffectRowID;

	//==========================================
	// OWNER INFO
	//==========================================

	// 이 아이템을 소유한 인벤토리 컴포넌트 
	UPROPERTY()
	class UInventoryComponent* OwningInventory;
	

	//==========================================
	// FUNCTIONS
	//==========================================

	/** 수량 설정 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(const int32 NewQuantity);

	/** 아이템 복사 생성 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	UItemBase* CreateItemCopy() const;

	/** 아이템 사용 (자식 클래스에서 오버라이드) */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void Use(class AStaffCharacter* Character);

	//==========================================
	// GETTERS
	//==========================================

	/** 전체 무게 (개당 무게 * 수량) */
	FORCEINLINE float GetItemStackWeight() const
	{
		return NumericData.Weight * Quantity;
	}

	/** 개당 무게 */
	FORCEINLINE float GetItemSingleWeight() const
	{
		return NumericData.Weight;
	}

	/** 스택 가능 여부 */
	FORCEINLINE bool IsStackable() const
	{
		return NumericData.bIsStackable && NumericData.MaxStackSize > 1;
	}

	/** 최대 스택 크기 */
	FORCEINLINE int32 GetMaxStackSize() const
	{
		return NumericData.MaxStackSize;
	}

	/** 스택이 꽉 찼는지 확인 */
	FORCEINLINE bool IsFullStack() const
	{
		return Quantity >= NumericData.MaxStackSize;
	}

	/** 내구도 시스템 사용 여부 */
	FORCEINLINE bool HasDurability() const
	{
		return NumericData.MaxDurability > 0.0f;
	}

//==========================================
// WEAPON-SPECIFIC DATA (무기 아이템용)
//==========================================

/** 현재 탄약 (무기 아이템 전용) */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Item|Weapon")
	int32 CurrentAmmo;

	/** 최대 탄약 (무기 아이템 전용) */
	UPROPERTY(BlueprintReadWrite, Category = "Item|Weapon")
	int32 MaxAmmo;

	//==========================================
	// WEAPON FUNCTIONS
	//==========================================

	/** 탄약 설정 */
	UFUNCTION(BlueprintCallable, Category = "Item|Weapon")
	void SetAmmo(int32 NewAmmo);

	/** 탄약 추가 */
	UFUNCTION(BlueprintCallable, Category = "Item|Weapon")
	void AddAmmo(int32 Amount);

	/** 탄약 완전 보급 */
	UFUNCTION(BlueprintCallable, Category = "Item|Weapon")
	void RefillAmmo();

	/** 탄약 비율 가져오기 */
	UFUNCTION(BlueprintPure, Category = "Item|Weapon")
	float GetAmmoPercent() const;


	//==========================================
	// OVERRIDE
	//==========================================

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};