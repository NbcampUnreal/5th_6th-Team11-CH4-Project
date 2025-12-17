// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "World/ItemDataStructs.h"
#include "UObject/NoExportTypes.h"
#include "ItemBase.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FItemTextData
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Name;             // 아이템 이름

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;      // 설명

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText InteractionText;  // "줍기", "사용" 같은 상호작용 문구
};

UCLASS()
class BIOPROTOCOL_API UItemBase : public UObject
{
	GENERATED_BODY()
	
public:
#pragma region Item Base Data
    /*
    UPROPERTY()
	UInventoryComponent* OwningInventory;
    */
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    FItemTextData TextData;

	UPROPERTY(VisibleAnywhere, Category = "Item Data", meta = (UIMin = "1", UIMax="100"))
    int Quantity;

    // 내부 ID (Key) - CSV/DataTable에서 RowName과 함께 사용
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;              // 예: "Pipe", "Pistol", "HealKit"

    // 화면에 보여줄 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;         // 예: 쇠파이프, 권총, 회복키트

    // 아이템 희귀도(등급)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemRarity Rarity;        // 하 / 중 / 상

    // 아이템 카테고리
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemCategory Category;    // Weapon / Work / Utility

    // 실제로 스폰할 액터 클래스 (월드에 떨어지는 아이템 액터 등)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> ItemClass;

    // 이 아이템이 가진 효과 데이터 RowID (없으면 NAME_None)
    // 예: 회복키트, 신호 교란기 등
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EffectRowID;         // FItemEffectData의 RowName과 매칭

    // 공용 숫자 데이터 묶음
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemNumericData NumericData;

    UPROPERTY(EditAnywhere)
    UTexture2D* Icon;

    UPROPERTY(EditAnywhere)
    UStaticMesh* Mesh;
#pragma endregion

	UItemBase();

    UFUNCTION(Category = "Item")
    UItemBase* CreateItemCopy() const;

    //무게를 사용할까..?
    UFUNCTION(Category = "Item")
    FORCEINLINE float GetItemStackWeight() const
    {
        return NumericData.Weight * Quantity;
	}
    UFUNCTION(Category = "Item")
    FORCEINLINE float GetSingleItemWeight() const
    {
        return NumericData.Weight;
    }
    UFUNCTION(Category = "Item")
    FORCEINLINE bool IsFullItemStack() const
    {
        return Quantity == NumericData.MaxStackSize;
	}

    UFUNCTION(Category = "Item")
    void SetQuantity(int NewQuantity);

    UFUNCTION(Category = "Item")
	virtual void UseItem(class ADXPlayerCharacter* Character);

protected:
    bool operator==(const FName& OtherID) const { return ItemID == OtherID; }
};
 