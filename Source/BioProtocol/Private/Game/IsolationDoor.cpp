// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/IsolationDoor.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/IsolationTimerWidget.h"
#include "Character/StaffStatusComponent.h"

AIsolationDoor::AIsolationDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrame"));
	RootComponent = DoorFrame;

	DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanel"));
	DoorPanel->SetupAttachment(RootComponent);

	TimerWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("TimerWidgetComp"));
	TimerWidgetComp->SetupAttachment(RootComponent);
	TimerWidgetComp->SetWidgetSpace(EWidgetSpace::World);
	TimerWidgetComp->SetDrawSize(FVector2D(200.0f, 100.0f));
	TimerWidgetComp->SetVisibility(false);

	JailSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("JailSpawnPoint"));
	JailSpawnPoint->SetupAttachment(RootComponent);
	JailSpawnPoint->SetRelativeLocation(FVector(0.0f, 200.0f, 100.0f));
}

void AIsolationDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AIsolationDoor, bIsOpen);
	DOREPLIFETIME(AIsolationDoor, Occupant);
}

void AIsolationDoor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OnRep_IsOpen();
	}
}

void AIsolationDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Occupant && !bIsOpen)
	{
		if (!TimerWidgetComp->IsVisible())
		{
			TimerWidgetComp->SetVisibility(true);
		}

		if (UStaffStatusComponent* Status = Occupant->FindComponentByClass<UStaffStatusComponent>())
		{
			if (UIsolationTimerWidget* TimerWidget = Cast<UIsolationTimerWidget>(TimerWidgetComp->GetUserWidgetObject()))
			{
				TimerWidget->UpdateTime(Status->JailTimer);
			}
		}
	}
	else
	{
		if (TimerWidgetComp->IsVisible())
		{
			TimerWidgetComp->SetVisibility(false);
		}
	}
}

void AIsolationDoor::SetDoorState(bool bNewOpen, APawn* NewOccupant)
{
	if (HasAuthority())
	{
		bIsOpen = bNewOpen;
		Occupant = NewOccupant;

		OnRep_IsOpen();

		if (bIsOpen)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Door] Opened. Room Cleared."));
		}
		else
		{
			FString Name = NewOccupant ? NewOccupant->GetName() : TEXT("Unknown");
			UE_LOG(LogTemp, Warning, TEXT("[Door] Closed. Isolating: %s"), *Name);
		}
	}
}

void AIsolationDoor::OnRep_IsOpen()
{
	UpdateDoorVisuals(bIsOpen);

	if (OnDoorStateChanged.IsBound())
	{
		OnDoorStateChanged.Broadcast(bIsOpen);
	}
}

FVector AIsolationDoor::GetJailSpawnLocation() const
{
	if (JailSpawnPoint)
	{
		return JailSpawnPoint->GetComponentLocation();
	}

	return GetActorLocation();
}