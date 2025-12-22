// InteractionInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionInterface.generated.h"

class ADXPlayerCharacter;

//==========================================
// ENUMS
//==========================================

UENUM(BlueprintType)
enum class EInteractableType : uint8
{
	Pickup              UMETA(DisplayName = "Pickup"),              // 습득 가능한 아이템
	MissionObject       UMETA(DisplayName = "MissionObject"),       // 미션 오브젝트 (수리 대상)
	SupplyStation       UMETA(DisplayName = "SupplyStation"),       // 보급소 (용접기 충전)
	DetentionLever      UMETA(DisplayName = "DetentionLever"),      // 격리실 레버
	EscapePod           UMETA(DisplayName = "EscapePod"),           // 탈출 포드
	Airlock             UMETA(DisplayName = "Airlock"),             // 에어락
	Door                UMETA(DisplayName = "Door"),                // 문
	Container           UMETA(DisplayName = "Container"),           // 상자/컨테이너
	Toggle              UMETA(DisplayName = "Toggle"),              // 스위치/토글
	NonPlayerCharacter  UMETA(DisplayName = "NonPlayerCharacter")   // NPC
};

//==========================================
// STRUCTS
//==========================================

USTRUCT(BlueprintType)
struct FInteractableData
{
	GENERATED_BODY()

	/** 상호작용 가능한 오브젝트의 타입 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Interaction")
	EInteractableType InteractableType;

	/** 오브젝트 이름 (UI에 표시) */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Interaction")
	FText Name;

	/** 상호작용 동작 설명 (예: "줍기", "수리하기", "열기") */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Interaction")
	FText Action;

	/** 수량 (Pickup 전용) */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Interaction")
	int32 Quantity;

	/** 상호작용 소요 시간 (초) - 0이면 즉시 실행 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Interaction")
	float InteractionDuration;

	/** 필요한 도구 타입 (MissionObject 전용) - NAME_None이면 도구 불필요 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Interaction")
	FName RequiredToolID;

	/** 상호작용 가능 여부 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Interaction")
	bool bCanInteract;

	/** 상호작용 범위 (미터) */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Interaction")
	float InteractionRange;

	// 생성자
	FInteractableData()
		: InteractableType(EInteractableType::Pickup)
		, Name(FText::GetEmpty())
		, Action(FText::FromString("Interact"))
		, Quantity(0)
		, InteractionDuration(0.0f)
		, RequiredToolID(NAME_None)
		, bCanInteract(true)
		, InteractionRange(200.0f)
	{
	}
};

//==========================================
// INTERFACE
//==========================================

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 가능한 모든 액터가 구현해야 하는 인터페이스
 * PickUp, MissionObject, SupplyStation 등에서 사용
 */
class BIOPROTOCOL_API IInteractionInterface
{
	GENERATED_BODY()

public:
	//==========================================
	// FOCUS (플레이어가 바라보기 시작/끝)
	//==========================================

	/**
	 * 플레이어가 이 오브젝트를 바라보기 시작할 때 호출
	 * 예: 외곽선 표시, UI 업데이트
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BeginFocus();
	virtual void BeginFocus_Implementation() {}

	/**
	 * 플레이어가 이 오브젝트에서 시선을 떼었을 때 호출
	 * 예: 외곽선 제거
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void EndFocus();
	virtual void EndFocus_Implementation() {}

	//==========================================
	// INTERACT (E키 누르기 시작/끝)
	//==========================================

	/**
	 * 플레이어가 E키를 누르기 시작할 때 호출
	 * 예: 진행 바 시작
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BeginInteract();
	virtual void BeginInteract_Implementation() {}

	/**
	 * 플레이어가 E키를 떼거나 상호작용이 취소될 때 호출
	 * 예: 진행 바 취소
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void EndInteract();
	virtual void EndInteract_Implementation() {}

	/**
	 * 상호작용 완료 시 호출 (InteractionDuration 경과 후)
	 * @param PlayerCharacter - 상호작용을 시도한 플레이어
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(ADXPlayerCharacter* PlayerCharacter);
	virtual void Interact_Implementation(ADXPlayerCharacter* PlayerCharacter) {}

	//==========================================
	// GETTERS
	//==========================================

	/**
	 * 상호작용 데이터 가져오기
	 * @return FInteractableData 구조체
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FInteractableData GetInteractableData() const;
	virtual FInteractableData GetInteractableData_Implementation() const
	{
		return InteractableData;
	}

	/**
	 * 상호작용 가능 여부 확인
	 * @param PlayerCharacter - 상호작용을 시도하는 플레이어
	 * @return 상호작용 가능하면 true
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(ADXPlayerCharacter* PlayerCharacter) const;
	virtual bool CanInteract_Implementation(ADXPlayerCharacter* PlayerCharacter) const
	{
		return InteractableData.bCanInteract;
	}

	//==========================================
	// DATA
	//==========================================

	/** 상호작용 데이터 (모든 구현 클래스가 이 변수를 가짐) */
	FInteractableData InteractableData;

};