// Fill out your copyright notice in the Description page of Project Settings.


#include "BioProtocol/Public/World/InterTestActor.h"

AInterTestActor::AInterTestActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(Mesh);
}

void AInterTestActor::BeginPlay()
{
}

void AInterTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AInterTestActor::BeginFocus()
{
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(true);
	}
}

void AInterTestActor::EndFocus()
{
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(false);
	}
}

void AInterTestActor::BeginInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("Calling BeginInteract override on interface test actor."));
}

void AInterTestActor::EndInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("Calling EndInteract override on interface test actor."));
}

void AInterTestActor::Interact()
{
	UE_LOG(LogTemp, Warning, TEXT("Calling Interact override on interface test actor."));
}

