// EquippableItem.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EquippableItem.generated.h"

class UItemBase;
class AStaffCharacter;
class USkeletalMeshComponent;

/**
 * 플레이어에게 장착 가능한 아이템의 베이스 클래스
 * 손, 등, 허리 등에 붙을 수 있는 실제 월드 액터
 */
UCLASS()
class BIOPROTOCOL_API AEquippableItem : public AActor
{
	GENERATED_BODY()

public:
	AEquippableItem();

	//==========================================
	// COMPONENTS
	//==========================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equippable | Components")
	UStaticMeshComponent* StaticMeshComp;

	/** 아이템의 메시 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equippable | Components")
	USkeletalMeshComponent* ItemMesh; // 손에 들었을 때 메쉬

	//==========================================
	// VARIABLES
	//==========================================

	/** 이 장착 아이템이 참조하는 인벤토리 데이터 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Equippable | Item Data")
	UItemBase* ItemReference;

	/** 현재 이 아이템을 소유한 캐릭터 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Equippable | Owner")
	AStaffCharacter* OwningCharacter;

	/** 장착 상태 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_IsEquipped, VisibleAnywhere, BlueprintReadOnly, Category = "Equippable | State")
	bool bIsEquipped;

	/** 현재 부착된 소켓 이름 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Equippable | Attachment")
	FName CurrentAttachedSocket;

	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void PlayUseAnimation();

	//==========================================
	// SOCKET NAMES
	//==========================================

	/** 손에 들 때 사용할 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable | Sockets")
	FName HandSocketName;

	/** 등에 맬 때 사용할 소켓 이름 (예: 샷건, 배터리) */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable | Sockets")
	FName BackSocketName;

	/** 허리에 맬 때 사용할 소켓 이름 (예: 권총) */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable | Sockets")
	FName HipSocketName;

	//==========================================
	// FUNCTIONS
	//==========================================

	/**
	 * 아이템 데이터로 초기화
	 * @param InItemReference - 인벤토리 아이템 데이터
	 * @param InOwner - 소유할 캐릭터
	 */
	void Initialize(UItemBase* InItemReference, AStaffCharacter* InOwner);

	/**
	 * 캐릭터의 특정 소켓에 아이템 부착
	 * @param SocketName - 부착할 소켓 이름
	 * @return 부착 성공 여부
	 */

	UFUNCTION(BlueprintCallable, Category = "Equippable")
	bool AttachToSocket(FName SocketName);

	/**
	 * 캐릭터의 손 소켓에 부착 (단축 함수)
	 */
	UFUNCTION(BlueprintCallable, Category = "Equippable")
	bool AttachToHand();

	/**
	 * 캐릭터의 등 소켓에 부착 (단축 함수)
	 */
	UFUNCTION(BlueprintCallable, Category = "Equippable")
	bool AttachToBack();

	/**
	 * 캐릭터의 허리 소켓에 부착 (단축 함수)
	 */
	UFUNCTION(BlueprintCallable, Category = "Equippable")
	bool AttachToHip();

	/**
	 * 캐릭터로부터 아이템 분리
	 */
	UFUNCTION(BlueprintCallable, Category = "Equippable")
	void Detach();

	/**
	 * 아이템 장착 (손에 들기)
	 */
	UFUNCTION(BlueprintCallable, Category = "Equippable")
	virtual void Equip();

	/**
	 * 아이템 해제 (등/허리로 이동)
	 */
	UFUNCTION(BlueprintCallable, Category = "Equippable")
	virtual void Unequip();

	// ========================================
	// USE ITEM (가상 함수)
	// ========================================
public:
	// 아이템 사용 (자식 클래스에서 오버라이드)
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void Use();

	// 아이템 사용 중지
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void StopUsing();

	// 사용 가능한지 체크
	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool CanUse() const;

protected:
	// 사용 중인지
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	bool bIsInUse;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Animation")
	UAnimMontage* UseMontage;

	//==========================================
	// SERVER RPC
	//==========================================

	/** 서버에 부착 요청 */
	UFUNCTION(Server, Reliable)
	void ServerAttachToSocket(FName SocketName);

	UFUNCTION(Server, Reliable)
	void ServerUse();

	UFUNCTION(Server, Reliable)
	void ServerStopUsing();

	/** 서버에 분리 요청 */
	UFUNCTION(Server, Reliable)
	void ServerDetach();

	/** 서버에 장착 요청 */
	UFUNCTION(Server, Reliable)
	void ServerEquip();

	/** 서버에 해제 요청 */
	UFUNCTION(Server, Reliable)
	void ServerUnequip();

	//==========================================
	// REPLICATION NOTIFY
	//==========================================

	UFUNCTION()
	void OnRep_IsEquipped();

	//==========================================
	// OVERRIDE
	//==========================================

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	/** 실제 부착 로직 (서버와 클라이언트 공통) */
	void PerformAttachment(FName SocketName);

	/** 실제 분리 로직 (서버와 클라이언트 공통) */
	void PerformDetachment();
};