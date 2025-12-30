#include "Daeho/DH_PickupItem.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include <Character/StaffCharacter.h>

ADH_PickupItem::ADH_PickupItem()
{
	bReplicates = true; // 서버-클라 동기화

	// 1. 충돌 박스 (루트)
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;
	CollisionBox->SetBoxExtent(FVector(30.f, 30.f, 30.f));

	// 캐릭터가 줍거나 라인트레이스에 맞아야 하므로 BlockAllDynamic
	CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 2. 메쉬
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	// 메쉬에는 충돌을 끕니다 (박스로만 판정)
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 기본값
	ItemType = EToolType::Wrench;
	Durability = 5;
}

// Called when the game starts or when spawned
void ADH_PickupItem::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateMesh();
}

void ADH_PickupItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADH_PickupItem, ItemType);
	DOREPLIFETIME(ADH_PickupItem, Durability);
}

void ADH_PickupItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdateMesh();
}

void ADH_PickupItem::InitializeDrop(EToolType Type, int32 NewDurability)
{
	ItemType = Type;
	Durability = NewDurability;

	UpdateMesh();
}

void ADH_PickupItem::OnRep_ItemType()
{
	UpdateMesh();
}

void ADH_PickupItem::UpdateMesh()
{
	// ItemType에 따라 메쉬 교체
	if (ItemType == EToolType::Wrench && WrenchMesh)
	{
		MeshComponent->SetStaticMesh(WrenchMesh);
	}
	else if (ItemType == EToolType::Welder && WelderMesh)
	{
		MeshComponent->SetStaticMesh(WelderMesh);
	}
}

void ADH_PickupItem::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority()) return;

	AStaffCharacter* MyChar = Cast<AStaffCharacter>(InstigatorPawn);
	if (MyChar)
	{
		if (MyChar->KServerPickUpItem(ItemType, Durability))
		{
			Destroy();
		}
	}
}



