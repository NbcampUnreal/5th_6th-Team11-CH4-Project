// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbyViewModel.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"

ALobbyViewModel::ALobbyViewModel()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MeshComp->SetRelativeLocation(FVector(0, 0, -90));
	MeshComp->SetRelativeRotation(FRotator(0, -90, 0));

	CaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComp"));
	CaptureComp->SetupAttachment(RootComponent);

	CaptureComp->SetRelativeLocation(FVector(200, 0, 0));
	CaptureComp->SetRelativeRotation(FRotator(0, 180, 0));

	CaptureComp->CaptureSource = ESceneCaptureSource::SCS_SceneColorSceneDepth;
	CaptureComp->bCaptureEveryFrame = true;
	CaptureComp->bCaptureOnMovement = false;

	LightComp = CreateDefaultSubobject<USpotLightComponent>(TEXT("LightComp"));
	LightComp->SetupAttachment(CaptureComp);
	LightComp->SetIntensity(5000.0f);
}

void ALobbyViewModel::BeginPlay()
{
	Super::BeginPlay();
}

UTextureRenderTarget2D* ALobbyViewModel::InitShowroom()
{
	RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, 256, 512);

	if (RenderTarget && CaptureComp)
	{
		RenderTarget->ClearColor = FLinearColor(0, 0, 0, 0);
		RenderTarget->UpdateResource();

		CaptureComp->TextureTarget = RenderTarget;
	}

	return RenderTarget;
}

void ALobbyViewModel::SetCharacterMaterial(UMaterialInterface* MatSlot0, UMaterialInterface* MatSlot1)
{
	if (MeshComp)
	{
		if (MatSlot0)
		{
			MeshComp->SetMaterial(0, MatSlot0);
		}
		if (MatSlot1)
		{
			MeshComp->SetMaterial(1, MatSlot1);
		}
	}
}