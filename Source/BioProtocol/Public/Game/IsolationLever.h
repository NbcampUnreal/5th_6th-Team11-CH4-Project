// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Daeho/MyInteractableInterface.h"
#include "IsolationLever.generated.h"

class UBoxComponent;
class AIsolationDoor;

UCLASS()
class BIOPROTOCOL_API AIsolationLever : public AActor, public IMyInteractableInterface
{
	GENERATED_BODY()
	
public:
	AIsolationLever();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* HandleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* InteractionBox;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Link")
	AIsolationDoor* TargetDoor;

	UPROPERTY(Replicated)
	bool bIsActive = false;

public:
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

protected:

	UFUNCTION()
	void OnDoorStateUpdated(bool bDoorOpen);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateLeverVisuals(bool bNeedRescue);

};