// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Daeho/MyInteractableInterface.h"
#include "BioEscapePod.generated.h"

class UBoxComponent;
class UWidgetComponent;
class UStaticMeshComponent;
class APawn;

UCLASS()
class BIOPROTOCOL_API ABioEscapePod : public AActor, public IMyInteractableInterface
{
	GENERATED_BODY()
	
public:
	ABioEscapePod();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PodMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* LeverMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* LeverInteractionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* WinTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* StatusWidget;

	UPROPERTY(ReplicatedUsing = OnRep_IsLeverReady, BlueprintReadOnly, Category = "State")
	bool bIsLeverReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsPodOpen, BlueprintReadOnly, Category = "State")
	bool bIsPodOpen = false;

	UFUNCTION()
	void OnWinTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_IsLeverReady();

	UFUNCTION()
	void OnRep_IsPodOpen();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdatePodVisuals(bool bReady, bool bOpen);

	UFUNCTION()
	void EnableLeverInteraction();
};
