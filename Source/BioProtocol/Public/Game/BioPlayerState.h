// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Game/BioProtocolTypes.h"
#include "BioPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerRoleChanged, EBioPlayerRole, NewRole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerTransformChanged, bool, bIsTransformed);

UCLASS()
class BIOPROTOCOL_API ABioPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ABioPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_GameRole, BlueprintReadOnly, Category = "Bio|Role")
	EBioPlayerRole GameRole;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Bio|Appearance")
	int32 ColorIndex;

	UPROPERTY(BlueprintAssignable, Category = "Bio|Event")
	FOnPlayerRoleChanged OnRoleChanged;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Bio|Ready")
	bool bIsReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_EOSPlayerName, BlueprintReadOnly, Category = "Bio|EOS")
	FString EOSPlayerName;

	// 블루프린트에서 역할 변경 요청
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Bio|Role")
	void Server_RequestRoleChange(EBioPlayerRole NewRole);

	// 서버에서 직접 역할 설정
	void Server_SetGameRole(EBioPlayerRole NewRole);

	// 서버에서 EOS 플레이어 이름 설정
	void Server_SetEOSPlayerName(const FString& InEOSPlayerName);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_GameRole();

	UFUNCTION()
	void OnRep_EOSPlayerName();

private:
	// EOS 플레이어 이름 초기화
	void TryInitEOSPlayerName();
	FTimerHandle EOSNameInitTimer;

	// 게임 인스턴스에서 역할 복원
	void RestoreRoleFromGameInstance();
};