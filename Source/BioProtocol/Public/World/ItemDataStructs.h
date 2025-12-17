
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemDataStructs.generated.h"

class AActor;

//아이템 희귀도
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Low    UMETA(DisplayName = "Low"),     // 하
    Medium UMETA(DisplayName = "Medium"),  // 중
    High   UMETA(DisplayName = "High")     // 상
};

// 아이템 종류 - 무기 / 업무 / 유틸리티
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
    Weapon   UMETA(DisplayName = "Weapon"),    // 무기 아이템
    Work     UMETA(DisplayName = "Work"),      // 업무(미션)용 아이템
    Utility  UMETA(DisplayName = "Utility")    // 유틸리티(소모품 등)
};

USTRUCT(BlueprintType)
struct FItemNumericData
{
    GENERATED_BODY();

    //아이템 무게 (인벤토리/이동속도)
    UPROPERTY(EditAnywhere)
    float Weight;

    //최대 스택 개수
    UPROPERTY(EditAnywhere)
    int32 MaxStackSize;

    //스택 가능 여부
    UPROPERTY(EditAnywhere)
    bool bIsStackable;
};

// 공통 아이템 데이터 (아이템 1개 = 이 Row 1줄)
// 인벤토리, 드랍 테이블, UI 등에서 공통으로 사용하는 정보
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
    GENERATED_BODY();

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
};

// 무기 아이템용 세부 데이터 (무기 전용 테이블에 사용 가능)
USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
    GENERATED_BODY();

    // 공통 FItemData의 ItemID와 매칭
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;        // 예: "Pipe", "Pistol", "Shotgun"

    // 공격력
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Damage;

    // 사거리 (근접이면 짧게, 총기는 길게 설정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Range;

    // 장탄수 (근접 무기는 -1 또는 0으로 무제한 표현)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MagazineSize;
};

// 업무(미션) 아이템용 세부 데이터
USTRUCT(BlueprintType)
struct FWorkItemData : public FTableRowBase
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;          // 예: "Wrench", "Welder", "Battery"

    // 양손만 사용 가능 여부 (배터리 등)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool  bTwoHandOnly;    // true면 양손 사용, 인벤토리 수납 제한 등

    // 내구도(연료) 시스템 사용 여부 (용접기 등)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool  bHasDurability;  // true면 아래 MaxDurability 사용

    // 최대 내구도 / 연료량
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxDurability;   // 예: Fuel 양

    // 이동 속도 배율 (배터리: 0.7f 등)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MoveSpeedMultiplier;

    // 사용/상호작용 시간 (E키 수리, 충전 시간 등)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UseTime;
};

// 유틸리티(소모품) 아이템용 세부 데이터
USTRUCT(BlueprintType)
struct FUtilityItemData : public FTableRowBase
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;          // 예: "HealKit", "AmmoPack", "SignalJammer"

    // 1회 사용 시 소모되는지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool  bConsumable;      // 회복키트, 탄약팩, 신호교란기: true

    // 효과 지속 시간 (초) - 즉시 효과면 0 또는 짧은 값
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Duration;         // 예: 신호 교란기 10초

    // 효과 범위 (AOE 반경, 미터 단위 가정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Radius;           // 예: 신호 교란기 10m

    // 자기 자신에게 효과가 적용되는지
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool  bAffectsSelf;     // 회복키트: true

    // 아군에게 효과가 적용되는지
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool  bAffectsAlly;     // 회복키트: true

    // AI에게 효과가 적용되는지
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool  bAffectsAI;       // 신호 교란기: true
};

// 아이템 효과 데이터 (지속적인 HP/스태미나/실드 변화를 정의)
// 버프/디버프용 별도 DataTable로 관리 가능
USTRUCT(BlueprintType)
struct FItemEffectData : public FTableRowBase
{
    GENERATED_BODY();

    // 이 효과를 사용하는 아이템 또는 효과 ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EffectID;                // 예: "HealFull", "JammerDebuff"

    // 초당 체력 변화량 (+면 회복, -면 피해)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HealthChangePerSecond;   // 예: 10.0f

    // 초당 스태미나 변화량
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StaminaChangePerSecond;  // 예: -5.0f

    // 초당 실드 변화량
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ShieldChangePerSecond;   // 예: 3.0f
};