// Fill out your copyright notice in the Description page of Project Settings.


#include "Equippable/EquippableUtility/EquippableUtility_AmmoPack.h"
#include "BioProtocol/Public/Character/StaffCharacter.h"
#include "BioProtocol/Public/Equippable/EquippableWeapon/EquippableWeapon_.h"
#include "BioProtocol/Public/Inventory/InventoryComponent.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "Kismet/GameplayStatics.h"

AEquippableUtility_AmmoPack::AEquippableUtility_AmmoPack()
{
	bConsumable = true;
	MaxUses = 1;
	Cooldown = 0.0f;

	bRefillAllWeapons = false;  // 기본: 현재 무기만 보급
	RefillParticle = nullptr;
}

void AEquippableUtility_AmmoPack::ProcessUtilityEffect()
{
	if (!OwningCharacter || !HasAuthority())
	{
		return;
	}

	bool bSuccess = false;
	int32 WeaponsRefilled = 0;

	if (bRefillAllWeapons)
	{
		// 모든 무기 보급
		WeaponsRefilled = RefillAllWeaponsInInventory();
		bSuccess = WeaponsRefilled > 0;

		UE_LOG(LogTemp, Log, TEXT("[AmmoPack] Refilled %d weapons"), WeaponsRefilled);
	}
	else
	{
		// 현재 장착된 무기만 보급
		bSuccess = RefillCurrentWeapon();

		if (bSuccess)
		{
			WeaponsRefilled = 1;
			UE_LOG(LogTemp, Log, TEXT("[AmmoPack] Refilled current weapon"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AmmoPack] No weapon to refill"));
		}
	}

	// 성공 시 이펙트 재생
	if (bSuccess)
	{
		// 파티클
		if (RefillParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				RefillParticle,
				OwningCharacter->GetActorLocation(),
				FRotator::ZeroRotator,
				FVector(1.5f)
			);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AmmoPack] Failed to refill - no weapons found"));
		}
	}
}
	//==========================================
	// REFILL CURRENT WEAPON
	//==========================================

	bool AEquippableUtility_AmmoPack::RefillCurrentWeapon()
	{
		if (!OwningCharacter)
		{
			return false;
		}

		// 현재 손에 들고 있는 아이템 가져오기
		AEquippableItem* CurrentItem = OwningCharacter->GetCurrentEquippedItem();
		if (!CurrentItem)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AmmoPack] No item equipped"));
			return false;
		}

		// 무기인지 확인
		AEquippableWeapon_* Weapon = Cast<AEquippableWeapon_>(CurrentItem);
		if (!Weapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AmmoPack] Equipped item is not a weapon"));
			return false;
		}

		// 근접 무기나 무한 탄약 무기는 보급 불필요
		if (Weapon->bIsMeleeWeapon || Weapon->bInfiniteAmmo)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AmmoPack] Weapon doesn't need ammo (melee or infinite)"));
			return false;
		}

		// 이미 풀탄창이면 보급 안함
		if (Weapon->CurrentAmmo >= Weapon->MaxAmmo)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AmmoPack] Weapon already at full ammo"));
			return false;
		}

		// 탄약 보급
		int32 AmmoBeforeRefill = Weapon->CurrentAmmo;
		Weapon->RefillAmmo();

		UE_LOG(LogTemp, Log, TEXT("[AmmoPack] Refilled %s: %d -> %d"),
			*Weapon->GetName(), AmmoBeforeRefill, Weapon->CurrentAmmo);

		return true;
	}
	//==========================================
// REFILL ALL WEAPONS IN INVENTORY
//==========================================

	int32 AEquippableUtility_AmmoPack::RefillAllWeaponsInInventory()
	{
		if (!OwningCharacter)
		{
			return 0;
		}

		UInventoryComponent* Inventory = OwningCharacter->GetInventory();
		if (!Inventory)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AmmoPack] No inventory component"));
			return 0;
		}

		int32 RefillCount = 0;

		// 방법 1: 현재 장착된 무기 액터들 찾기
		TArray<AEquippableWeapon_*> EquippedWeapons = FindAllEquippedWeapons();

		for (AEquippableWeapon_* Weapon : EquippedWeapons)
		{
			if (!Weapon || Weapon->bIsMeleeWeapon || Weapon->bInfiniteAmmo)
			{
				continue;
			}

			if (Weapon->CurrentAmmo < Weapon->MaxAmmo)
			{
				int32 AmmoBeforeRefill = Weapon->CurrentAmmo;
				Weapon->RefillAmmo();

				UE_LOG(LogTemp, Log, TEXT("[AmmoPack] Refilled equipped weapon: %s (%d -> %d)"),
					*Weapon->GetName(), AmmoBeforeRefill, Weapon->CurrentAmmo);

				RefillCount++;
			}
		}

		// 방법 2: 인벤토리의 모든 무기 아이템 탄약 데이터 업데이트
		const TArray<UItemBase*>& AllItems = Inventory->GetAllItems();

		for (UItemBase* Item : AllItems)
		{
			if (!Item || Item->Category != EItemCategory::Weapon)
			{
				continue;
			}

			// 무기 아이템의 탄약 데이터를 MaxAmmo로 설정
			// (실제 장착되지 않은 무기는 데이터만 업데이트)
			if (Item->NumericData.Durability < Item->NumericData.MaxDurability)
			{
				// NumericData.Durability를 탄약 저장용으로 사용
				// (또는 별도 AmmoCount 변수 추가 필요)
				Item->NumericData.Durability = Item->NumericData.MaxDurability;

				UE_LOG(LogTemp, Log, TEXT("[AmmoPack] Refilled weapon data: %s"),
					*Item->TextData.Name.ToString());

				RefillCount++;
			}
		}

		return RefillCount;
	}
//==========================================
// FIND ALL EQUIPPED WEAPONS
//==========================================

	TArray<AEquippableWeapon_*> AEquippableUtility_AmmoPack::FindAllEquippedWeapons()
	{
		TArray<AEquippableWeapon_*> Weapons;

		if (!OwningCharacter)
		{
			return Weapons;
		}

		// 현재 손에 들고 있는 무기
		AEquippableItem* CurrentItem = OwningCharacter->GetCurrentEquippedItem();
		if (CurrentItem)
		{
			AEquippableWeapon_* CurrentWeapon = Cast<AEquippableWeapon_>(CurrentItem);
			if (CurrentWeapon)
			{
				Weapons.Add(CurrentWeapon);
			}
		}

		// 등/허리에 장착된 무기들 찾기
		TArray<AActor*> AttachedActors;
		OwningCharacter->GetAttachedActors(AttachedActors);

		for (AActor* Actor : AttachedActors)
		{
			AEquippableWeapon_* Weapon = Cast<AEquippableWeapon_>(Actor);
			if (Weapon && !Weapons.Contains(Weapon))
			{
				Weapons.Add(Weapon);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[AmmoPack] Found %d equipped weapons"), Weapons.Num());

		return Weapons;

}