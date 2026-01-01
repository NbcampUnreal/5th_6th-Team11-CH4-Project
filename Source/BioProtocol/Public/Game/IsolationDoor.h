// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsolationDoor.generated.h"

class UWidgetComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoorStateChanged, bool, bIsDoorOpen);

UCLASS()
class BIOPROTOCOL_API AIsolationDoor : public AActor
{
	GENERATED_BODY()
	
public:
	AIsolationDoor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DoorFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DoorPanel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* TimerWidgetComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* JailSpawnPoint;


	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	APawn* Occupant = nullptr;

public:
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnDoorStateChanged OnDoorStateChanged;

	UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "State")
	bool bIsOpen = true;

	void SetDoorState(bool bNewOpen, APawn* NewOccupant);
	bool IsOccupied() const { return Occupant != nullptr; }
	APawn* GetOccupant() const { return Occupant; }
	FVector GetJailSpawnLocation() const;

protected:
	UFUNCTION()
	void OnRep_IsOpen();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateDoorVisuals(bool bOpen);
};