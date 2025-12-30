// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyInteractableInterface.h"
#include "TestCharacter.h"
#include "TaskObject.generated.h"

// ?꾨갑 ?좎뼵
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
	// ?곹깭 蹂듭젣???⑥닔
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// ?명꽣?섏씠???⑥닔 援ы쁽
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

protected:
	// ?꾨Т媛 ?대? ?꾨즺?섏뿀?붿? ?뺤씤 (RepNotify瑜??⑥꽌 ?쒓컖???④낵 ?숆린??媛??
	UPROPERTY(ReplicatedUsing = OnRep_IsCompleted, BlueprintReadOnly, Category = "Task")
	bool bIsCompleted;

	// 蹂??媛믪씠 蹂寃쎈릺?덉쓣 ???대씪?댁뼵?몄뿉???몄텧??
	UFUNCTION()
	void OnRep_IsCompleted();

	/** 異⑸룎 媛먯???諛뺤뒪 而댄룷?뚰듃 (?덉뿉 ??蹂댁엫, ?덉씠? 留욌뒗 ?⑸룄) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;

	/** ?뚮???諛섏쭩?대뒗 ?댄럺?몄슜 而댄룷?뚰듃 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* NiagaraEffect;

public:
	/** ???꾨Т瑜??섑뻾?섍린 ?꾪빐 ?꾩슂???꾧뎄 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
	EToolType RequiredTool;
};
