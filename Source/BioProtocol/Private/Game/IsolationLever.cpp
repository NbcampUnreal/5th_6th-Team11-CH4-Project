// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/IsolationLever.h"
#include "Game/IsolationDoor.h"
#include "Game/BioGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

AIsolationLever::AIsolationLever()
{
	bReplicates = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(RootComponent);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	InteractionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void AIsolationLever::BeginPlay()
{
	Super::BeginPlay();

	if (TargetDoor)
	{
		TargetDoor->OnDoorStateChanged.AddDynamic(this, &AIsolationLever::OnDoorStateUpdated);

		OnDoorStateUpdated(TargetDoor->bIsOpen);
	}
}

void AIsolationLever::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AIsolationLever, bIsActive);
}

void AIsolationLever::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority()) return;
	if (!TargetDoor) return;

	if (!TargetDoor->IsOccupied()) return;

	if (ABioGameMode* GM = Cast<ABioGameMode>(GetWorld()->GetAuthGameMode()))
	{
		APawn* Victim = TargetDoor->GetOccupant();
		if (Victim)
		{
			GM->ReleasePlayerFromJail(Victim);
		}
	}
}

void AIsolationLever::OnDoorStateUpdated(bool bDoorOpen)
{
	bool bNeedRescue = !bDoorOpen;

	UpdateLeverVisuals(bNeedRescue);
}