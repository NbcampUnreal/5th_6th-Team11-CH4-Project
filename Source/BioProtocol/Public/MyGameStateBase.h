// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MyGameStateBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTaskProgressChanged, float, NewProgress);

UCLASS()
class BIOPROTOCOL_API AMyGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	AMyGameStateBase();

	// 변수 복제를 위해 필요한 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 전체 임무 개수 (서버만 수정, 클라는 읽기만)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mafia Game")
	int32 TotalTasks;

	// 완료된 임무 개수 (서버만 수정, 클라는 읽기만)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mafia Game")
	int32 CompletedTasks;

	// 임무 진행 상황 업데이트 (서버에서 호출)
	void AddCompletedTask();

	// 현재 진행률(0.0 ~ 1.0) 반환 헬퍼 함수
	UFUNCTION(BlueprintCallable, Category = "Mafia Game")
	float GetTaskProgress() const;
};
