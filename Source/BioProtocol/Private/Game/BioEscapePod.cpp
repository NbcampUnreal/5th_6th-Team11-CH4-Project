// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioEscapePod.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Game/BioGameState.h"
#include "Game/BioGameMode.h"
#include "Game/BioPlayerState.h"
#include "Kismet/GameplayStatics.h"

ABioEscapePod::ABioEscapePod()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	PodMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PodMesh"));
	RootComponent = PodMesh;

	LeverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeverMesh"));
	LeverMesh->SetupAttachment(RootComponent);

	LeverInteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeverInteractionBox"));
	LeverInteractionBox->SetupAttachment(LeverMesh);
	LeverInteractionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	LeverInteractionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	WinTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("WinTrigger"));
	WinTrigger->SetupAttachment(RootComponent);
	WinTrigger->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	WinTrigger->SetCollisionProfileName(TEXT("Trigger"));
	WinTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StatusWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusWidget"));
	StatusWidget->SetupAttachment(RootComponent);
}

void ABioEscapePod::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABioEscapePod, bIsLeverReady);
	DOREPLIFETIME(ABioEscapePod, bIsPodOpen);
}

void ABioEscapePod::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		WinTrigger->OnComponentBeginOverlap.AddDynamic(this, &ABioEscapePod::OnWinTriggerOverlap);

		if (ABioGameState* GS = GetWorld()->GetGameState<ABioGameState>())
		{
			if (GS->bCanEscape)
			{
				EnableLeverInteraction();
			}
			else
			{
				GS->OnEscapeEnabled.AddDynamic(this, &ABioEscapePod::EnableLeverInteraction);
			}
		}
	}
}


void ABioEscapePod::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority()) return;

	if (!bIsLeverReady)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EscapePod] Mission NOT complete yet. Lever Locked."));
		return;
	}

	if (bIsPodOpen) return;

	UE_LOG(LogTemp, Warning, TEXT("[EscapePod] Lever Pulled! Opening Pod..."));

	bIsPodOpen = true;

	WinTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	OnRep_IsPodOpen();
}

void ABioEscapePod::OnWinTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsPodOpen) return;

	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!PlayerPawn) return;

	ABioPlayerState* PS = PlayerPawn->GetPlayerState<ABioPlayerState>();

	if (PS && PS->GameRole == EBioPlayerRole::Staff)
	{
		UE_LOG(LogTemp, Error, TEXT("!!! STAFF ESCAPED !!!"));

		if (ABioGameMode* GM = Cast<ABioGameMode>(GetWorld()->GetAuthGameMode()))
		{
			if (ABioGameState* GS = GetWorld()->GetGameState<ABioGameState>())
			{
				GS->SetGamePhase(EBioGamePhase::End);
			}
			GM->EndGame();
		}
	}
}

void ABioEscapePod::EnableLeverInteraction()
{
	if (bIsLeverReady) return;

	bIsLeverReady = true;
	OnRep_IsLeverReady();

	UE_LOG(LogTemp, Warning, TEXT("[EscapePod] Signal Received: Lever UNLOCKED!"));
}

void ABioEscapePod::OnRep_IsLeverReady()
{
	UpdatePodVisuals(bIsLeverReady, bIsPodOpen);
}

void ABioEscapePod::OnRep_IsPodOpen()
{
	UpdatePodVisuals(bIsLeverReady, bIsPodOpen);
}

