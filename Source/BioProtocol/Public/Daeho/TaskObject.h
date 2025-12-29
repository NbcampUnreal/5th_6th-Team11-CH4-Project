// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyInteractableInterface.h"
#include "TestCharacter.h"
#include "TaskObject.generated.h"

// 전방 선언
class UBoxComponent;
class UNiagaraComponent;

UCLASS()
class BIOPROTOCOL_API ATaskObject : public AActor, public IMyInteractableInterface
{
	GENERATED_BODY()
	
public:
	ATaskObject();

protected:
	virtual void BeginPlay() override;
	// 상태 복제용 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 인터페이스 함수 구현
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

protected:
	// 임무가 이미 완료되었는지 확인 (RepNotify를 써서 시각적 효과 동기화 가능)
	UPROPERTY(ReplicatedUsing = OnRep_IsCompleted, BlueprintReadOnly, Category = "Task")
	bool bIsCompleted;

	// 변수 값이 변경되었을 때 클라이언트에서 호출됨
	UFUNCTION()
	void OnRep_IsCompleted();

	/** 충돌 감지용 박스 컴포넌트 (눈에 안 보임, 레이저 맞는 용도) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;

	/** 파란색 반짝이는 이펙트용 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* NiagaraEffect;

public:
	/** 이 임무를 수행하기 위해 필요한 도구 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
	EToolType RequiredTool;
};
