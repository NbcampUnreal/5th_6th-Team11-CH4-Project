// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LobbyViewModel.generated.h"

class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class UTextureRenderTarget2D;
class USpotLightComponent;

UCLASS()
class BIOPROTOCOL_API ALobbyViewModel : public AActor
{
	GENERATED_BODY()

public:
	ALobbyViewModel();

	UTextureRenderTarget2D* InitShowroom();

	void SetCharacterMaterial(UMaterialInterface* MatSlot0, UMaterialInterface* MatSlot1);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneCaptureComponent2D* CaptureComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USpotLightComponent* LightComp;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;
};
