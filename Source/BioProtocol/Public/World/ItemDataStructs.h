// ItemDataStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "ItemDataStructs.generated.h"

class AActor;
//class AEquippableItem;

//==========================================
// ENUMS
//==========================================

// 아이템 희귀도
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Low    UMETA(DisplayName = "Low"),     // 하
    Medium UMETA(DisplayName = "Medium"),  // 중
    High   UMETA(DisplayName = "High")     // 상
};

// 아이템 카테고리 (무기 / 업무 / 유틸리티)
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
    Weapon   UMETA(DisplayName = "Weapon"),    // 무기 아이템
    Work     UMETA(DisplayName = "Work"),      // 업무(미션)용 아이템
    Utility  UMETA(DisplayName = "Utility")    // 유틸리티(소모품 등)
};

// 아이템 타입 (툴/무기/유틸/소모품)
UENUM(BlueprintType)
enum class EItemType : uint8
{
    Tool,
    Weapon,
    Utility,
    Consumable
};

UENUM(BlueprintType)
enum class EItemQuality : uint8
{
    Common,
    Uncommon,
    Rare,
    Epic
};

//==========================================
// BASIC STRUCTS - 숫자 데이터
//==========================================

USTRUCT(BlueprintType)
struct FItemNumericData
{
    GENERATED_BODY()

    /** 아이템 무게 (인벤토리/이동속도) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Weight;

    /** 최대 스택 개수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStackSize;

    /** 스택 가능 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsStackable;

    /** 내구도 (용접기용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Durability;

    /** 최대 내구도 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxDurability;

    /** 이동속도 감소 비율 (0.0 ~ 1.0) - 배터리는 0.5 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementSpeedModifier;

    FItemNumericData()
        : Weight(1.0f)
        , MaxStackSize(1)
        , bIsStackable(false)
        , Durability(100.0f)
        , MaxDurability(100.0f)
        , MovementSpeedModifier(1.0f)
    {
    }
};

//==========================================
// TEXT DATA - UI에 표시할 텍스트
//==========================================

USTRUCT(BlueprintType)
struct FItemTextData
{
    GENERATED_BODY()

    /** 화면에 표시될 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Name;

    /** 아이템 설명 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    /** 상호작용 텍스트 (예: "줍기", "사용하기") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText InteractionText;

    /** 사용 방법 설명 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText UsageText;

    FItemTextData()
        : Name(FText::FromString("Item"))
        , Description(FText::FromString("No Description"))
        , InteractionText(FText::FromString("Pick Up"))
        , UsageText(FText::FromString(""))
    {
    }
};

//==========================================
// ASSET DATA - 메시, 아이콘 등
//==========================================

USTRUCT(BlueprintType)
struct FItemAssetData
{
    GENERATED_BODY()

    /** 아이템 아이콘 (UI용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* Icon;

    /** 월드에 떨어질 때 사용할 스태틱 메시 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMesh* Mesh;

    /** 장착했을 때 사용할 스켈레탈 메시 (손에 들 때) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USkeletalMesh* SkeletalMesh;

    FItemAssetData()
        : Icon(nullptr)
        , Mesh(nullptr)
        , SkeletalMesh(nullptr)
    {
    }
};

//==========================================
// MAIN ITEM DATA - 공통 아이템 데이터
//==========================================

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
    GENERATED_BODY()

    /** 내부 ID (Key) - DataTable의 RowName과 일치 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    /** 화면에 보여줄 이름 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    /** 아이템 희귀도(등급) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemRarity Rarity;

    /** 아이템 카테고리 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemCategory Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemType ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemQuality ItemQuality;

    // 실제로 스폰할 장착 가능한 액터 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AEquippableItem> ItemClass; 

    /** 이 아이템이 가진 효과 데이터 RowID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EffectRowID;

    /** 숫자 데이터 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemNumericData NumericData;

    /** 텍스트 데이터 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemTextData TextData;

    /** 에셋 데이터 (아이콘, 메시) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemAssetData AssetData;

    FItemData()
        : ItemID(NAME_None)
        , DisplayName(FText::FromString("Item"))
        , Rarity(EItemRarity::Low)
        , Category(EItemCategory::Work)
        , ItemType(EItemType::Tool)
        , ItemQuality(EItemQuality::Common)
       // , ItemClass(nullptr)
        , EffectRowID(NAME_None)
    { }
};

//==========================================
// WEAPON DATA - 무기 전용 세부 데이터
//==========================================

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
    GENERATED_BODY()

    /** FItemData의 ItemID와 매칭 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    /** 공격력 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Damage;

    /** 사거리 (근접이면 짧게, 총기는 길게) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Range;

    /** 장탄수 (근접 무기는 -1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MagazineSize;

    /** 연사 속도 (초당 발사 수) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FireRate;

    FWeaponData()
        : ItemID(NAME_None)
        , Damage(10.0f)
        , Range(100.0f)
        , MagazineSize(10)
        , FireRate(1.0f)
    {
    }
};

//==========================================
// WORK ITEM DATA - 업무 아이템 세부 데이터
//==========================================

USTRUCT(BlueprintType)
struct FWorkItemData : public FTableRowBase
{
    GENERATED_BODY()

    /** FItemData의 ItemID와 매칭 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    /** 양손만 사용 가능 여부 (배터리) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTwoHandOnly;

    /** 내구도(연료) 시스템 사용 여부 (용접기) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasDurability;

    /** 최대 내구도/연료량 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxDurability;

    /** 이동 속도 배율 (배터리: 0.5f) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MoveSpeedMultiplier;

    /** 사용/상호작용 시간 (E키 수리 시간) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UseTime;

    FWorkItemData()
        : ItemID(NAME_None)
        , bTwoHandOnly(false)
        , bHasDurability(false)
        , MaxDurability(100.0f)
        , MoveSpeedMultiplier(1.0f)
        , UseTime(1.0f)
    {
    }
};

//==========================================
// UTILITY ITEM DATA - 유틸리티 아이템 세부 데이터
//==========================================

USTRUCT(BlueprintType)
struct FUtilityItemData : public FTableRowBase
{
    GENERATED_BODY()

    /** FItemData의 ItemID와 매칭 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    /** 1회 사용 시 소모되는지 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bConsumable;

    /** 효과 지속 시간 (초) - 신호교란기 10초 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Duration;

    /** 효과 범위 (AOE 반경, 미터) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Radius;

    /** 자기 자신에게 효과 적용 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAffectsSelf;

    /** 아군에게 효과 적용 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAffectsAlly;

    /** AI에게 효과 적용 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAffectsAI;

    FUtilityItemData()
        : ItemID(NAME_None)
        , bConsumable(true)
        , Duration(0.0f)
        , Radius(0.0f)
        , bAffectsSelf(true)
        , bAffectsAlly(false)
        , bAffectsAI(false)
    {
    }
};

//==========================================
// ITEM EFFECT DATA - 아이템 효과 데이터
//==========================================

USTRUCT(BlueprintType)
struct FItemEffectData : public FTableRowBase
{
    GENERATED_BODY()

    /** 효과 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EffectID;

    /** 초당 체력 변화량 (+면 회복, -면 피해) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HealthChangePerSecond;

    /** 초당 스태미나 변화량 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StaminaChangePerSecond;

    /** 초당 실드 변화량 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ShieldChangePerSecond;

    FItemEffectData()
        : EffectID(NAME_None)
        , HealthChangePerSecond(0.0f)
        , StaminaChangePerSecond(0.0f)
        , ShieldChangePerSecond(0.0f)
    {
    }
};