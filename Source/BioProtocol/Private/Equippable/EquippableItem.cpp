#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/StaffCharacter.h"

AEquippableItem::AEquippableItem()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ItemMesh->CastShadow = true;
	SetRootComponent(ItemMesh);

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComp"));
	StaticMeshComp->SetupAttachment(ItemMesh);
	StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	StaticMeshComp->CastShadow = true;
	StaticMeshComp->SetVisibility(false);

	HandSocketName = FName("hand_r_socket");
	BackSocketName = FName("spine_03_socket");
	HipSocketName = FName("pelvis_socket");

	bIsEquipped = false;
	bIsInUse = false;
	CurrentAttachedSocket = NAME_None;
	OwningCharacter = nullptr;
	ItemReference = nullptr;
}

void AEquippableItem::BeginPlay()
{
	Super::BeginPlay();

	const FName RowToUse = (ItemRowName.IsNone() ? ItemID : ItemRowName);

	if (ItemDataTable && !RowToUse.IsNone())
	{
		static const FString Context(TEXT("EquippableItem_LoadItemData"));

		if (const FItemData* Row = ItemDataTable->FindRow<FItemData>(RowToUse, Context, true))
		{
			// 1) 월드에 떨어져 있을 때 사용할 스태틱 메시
			if (Row->AssetData.Mesh)
			{
				StaticMeshComp->SetStaticMesh(Row->AssetData.Mesh);
			}

			// 2) 손에 들었을 때 사용할 스켈레탈 메시
			if (Row->AssetData.SkeletalMesh)
			{
				ItemMesh->SetSkeletalMesh(Row->AssetData.SkeletalMesh);
			}

			// 3) (선택) 아이템 이름/설명 등은 UItemBase나 UI 쪽에서 사용
			// 예: 로그로 확인
			UE_LOG(LogTemp, Log, TEXT("EquippableItem '%s' loaded: %s"),
				*GetName(),
				*Row->TextData.Name.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AEquippableItem: Row '%s' not found in DataTable '%s'"),
				*RowToUse.ToString(),
				*ItemDataTable->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("AEquippableItem: ItemDataTable or ItemID/ItemRowName not set on %s"),
			*GetName());
	}
}

void AEquippableItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEquippableItem, ItemReference);
	DOREPLIFETIME(AEquippableItem, OwningCharacter);
	DOREPLIFETIME(AEquippableItem, bIsEquipped);
	DOREPLIFETIME(AEquippableItem, bIsInUse);
	DOREPLIFETIME(AEquippableItem, CurrentAttachedSocket);
}

// ========================================
// USE ITEM FUNCTIONS
// ========================================

void AEquippableItem::Use()
{
	if (!HasAuthority())
	{
		ServerUse();
		return;
	}

	bIsInUse = true;
	/*
	if (!CanUse())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Item] Cannot use: %s"), *GetName());
		return;
	}
	*/

	bIsInUse = true;
	UE_LOG(LogTemp, Log, TEXT("[Item] Base Use called: %s"), *GetName());
}

void AEquippableItem::StopUsing()
{
	if (!HasAuthority())
	{
		ServerStopUsing();
		return;
	}

	bIsInUse = false;
	UE_LOG(LogTemp, Log, TEXT("[Item] Stopped using: %s"), *GetName());
}

bool AEquippableItem::CanUse() const
{
	return bIsEquipped && OwningCharacter != nullptr;
}

void AEquippableItem::ServerUse_Implementation()
{
	Use();
}

void AEquippableItem::ServerStopUsing_Implementation()
{
	StopUsing();
}

void AEquippableItem::PlayUseAnimation()
{
	// 1. 몽타주가 설정되어 있는지 확인
	if (!UseMontage)
	{
		return;
	}

	// 2. 이 아이템을 들고 있는 캐릭터 가져오기 (OwningCharacter 같은 멤버를 두고 있다면 그걸 사용)
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	// 3. 메쉬의 AnimInstance에서 몽타주 재생
	if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(UseMontage);
	}
}


// ========================================
// INITIALIZATION
// ========================================

void AEquippableItem::Initialize(UItemBase* InItemReference, AStaffCharacter* InOwner)
{
	if (!InItemReference || !InOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("[EquippableItem] Initialize failed: Invalid parameters"));
		return;
	}

	ItemReference = InItemReference;
	OwningCharacter = InOwner;

	// 1. 스태틱 메쉬 사용 (예: StaticMeshComponent가 있을 때)
	if (StaticMeshComp && ItemReference->AssetData.Mesh)
	{
		// ItemMesh가 UStaticMeshComponent* 라면
		StaticMeshComp->SetStaticMesh(ItemReference->AssetData.Mesh);
		StaticMeshComp->SetVisibility(true);
		ItemMesh->SetVisibility(false);
	}

	// 2. 스켈레탈 메쉬 사용 (손에 드는 무기/도구용 예시)
	if (ItemMesh && ItemReference->AssetData.SkeletalMesh)
	{
		ItemMesh->SetSkeletalMesh(ItemReference->AssetData.SkeletalMesh);
		ItemMesh->SetVisibility(true);
		StaticMeshComp->SetVisibility(false);
	}

	UE_LOG(LogTemp, Log, TEXT("[EquippableItem] Initialized: %s for %s"),
		*ItemReference->TextData.Name.ToString(),
		*OwningCharacter->GetName());
}

// ========================================
// ATTACHMENT FUNCTIONS
// ========================================

bool AEquippableItem::AttachToSocket(FName SocketName)
{
	if (!HasAuthority())
	{
		ServerAttachToSocket(SocketName);
		return true;
	}

	PerformAttachment(SocketName);
	return true;
}

bool AEquippableItem::AttachToHand()
{
	return AttachToSocket(HandSocketName);
}

bool AEquippableItem::AttachToBack()
{
	return AttachToSocket(BackSocketName);
}

bool AEquippableItem::AttachToHip()
{
	return AttachToSocket(HipSocketName);
}

void AEquippableItem::Detach()
{
	if (!HasAuthority())
	{
		ServerDetach();
		return;
	}

	PerformDetachment();
}

void AEquippableItem::Equip()
{
	if (!HasAuthority())
	{
		ServerEquip();
		return;
	}

	PerformAttachment(HandSocketName);
	bIsEquipped = true;

	UE_LOG(LogTemp, Log, TEXT("[EquippableItem] Equipped: %s"), *GetName());
}

void AEquippableItem::Unequip()
{
	if (!HasAuthority())
	{
		ServerUnequip();
		return;
	}

	FName TargetSocket = BackSocketName;

	if (ItemReference)
	{
		switch (ItemReference->ItemType)
		{
		case EItemType::Tool:
		case EItemType::Weapon:
			TargetSocket = BackSocketName;
			break;

		case EItemType::Utility:
			TargetSocket = HipSocketName;
			break;

		default:
			TargetSocket = BackSocketName;
			break;
		}
	}

	PerformAttachment(TargetSocket);
	bIsEquipped = false;

	UE_LOG(LogTemp, Log, TEXT("[EquippableItem] Unequipped: %s to %s"),
		*GetName(), *TargetSocket.ToString());
}

// ========================================
// CORE ATTACHMENT LOGIC
// ========================================

void AEquippableItem::PerformAttachment(FName SocketName)
{
	if (!OwningCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("[EquippableItem] PerformAttachment failed: No OwningCharacter"));
		return;
	}

	USkeletalMeshComponent* CharacterMesh = OwningCharacter->GetMesh();
	if (!CharacterMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[EquippableItem] PerformAttachment failed: No Character Mesh"));
		return;
	}

	if (!CharacterMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogTemp, Error, TEXT("[EquippableItem] Socket not found: %s"), *SocketName.ToString());
		return;
	}

	if (GetAttachParentActor())
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	FAttachmentTransformRules AttachRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld,
		true
	);

	AttachToComponent(CharacterMesh, AttachRules, SocketName);
	CurrentAttachedSocket = SocketName;

	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UE_LOG(LogTemp, Log, TEXT("[EquippableItem] Attached %s to socket: %s"),
		*GetName(), *SocketName.ToString());
}

void AEquippableItem::PerformDetachment()
{
	if (!GetAttachParentActor())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquippableItem] Already detached"));
		return;
	}

	FDetachmentTransformRules DetachRules(
		EDetachmentRule::KeepWorld,
		true
	);

	DetachFromActor(DetachRules);
	CurrentAttachedSocket = NAME_None;

	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetSimulatePhysics(true);

	UE_LOG(LogTemp, Log, TEXT("[EquippableItem] Detached: %s"), *GetName());
}

// ========================================
// SERVER RPC
// ========================================

void AEquippableItem::ServerAttachToSocket_Implementation(FName SocketName)
{
	PerformAttachment(SocketName);
}

void AEquippableItem::ServerDetach_Implementation()
{
	PerformDetachment();
}

void AEquippableItem::ServerEquip_Implementation()
{
	Equip();
}

void AEquippableItem::ServerUnequip_Implementation()
{
	Unequip();
}

// ========================================
// REPLICATION NOTIFY
// ========================================

void AEquippableItem::OnRep_IsEquipped()
{
	if (bIsEquipped)
	{
		UE_LOG(LogTemp, Log, TEXT("[EquippableItem Client] Item equipped: %s"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[EquippableItem Client] Item unequipped: %s"), *GetName());
	}
}
