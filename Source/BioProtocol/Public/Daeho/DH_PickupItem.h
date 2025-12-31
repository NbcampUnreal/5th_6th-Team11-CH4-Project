// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Daeho/MyInteractableInterface.h"
#include "Daeho/TestCharacter.h"
#include "DH_PickupItem.generated.h"

class UBoxComponent;

UCLASS()
class BIOPROTOCOL_API ADH_PickupItem : public AActor, public IMyInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADH_PickupItem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 에디터에서 값이 바뀌면 실시간으로 메쉬를 업데이트하기 위함 (선택사항)
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// 인터페이스 구현: E키를 누르면 호출됨
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

	// 아이템의 속성을 설정하는 함수 (버릴 때 사용)
	void InitializeDrop(EToolType Type, int32 NewDurability);

protected:
	// 충돌 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;

	// 시각적 메쉬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

public:
	// [중요] ReplicatedUsing을 추가하여, 클라이언트도 변수가 바뀌면 메쉬를 업데이트하게 함
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_ItemType, BlueprintReadWrite, Category = "Item Stats")
	EToolType ItemType;

	// 현재 내구도 (에디터에서 초기값 설정)
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "Item Stats")
	int32 Durability;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UStaticMesh* WrenchMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UStaticMesh* WelderMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UStaticMesh* GunMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UStaticMesh* PotionMesh;
	UFUNCTION()
	void OnRep_ItemType(); // 클라이언트용

	void UpdateMesh(); // 실제 메쉬 변경 로직
};
