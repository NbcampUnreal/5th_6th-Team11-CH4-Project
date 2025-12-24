// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyStatusChanged);

UCLASS()
class BIOPROTOCOL_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnLobbyStatusChanged OnStatusChanged;

	UPROPERTY(ReplicatedUsing = OnRep_IsReady, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentColorIndex, BlueprintReadOnly, Category = "Lobby")
	int32 CurrentColorIndex = -1;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerToggleReady();

	UFUNCTION(Server, Reliable)
	void ServerSetColorIndex(int32 NewIndex);

	//UFUNCTION(BlueprintCallable, Category = "Lobby")
	//FString GetNickname() const { return Nickname; }

	//UFUNCTION(BlueprintCallable, Category = "Lobby")
	//void SetNickname(const FString& InName);

	//UFUNCTION(Server, Reliable)
	//void ServerSetNickname(const FString& InName);

	//UPROPERTY(ReplicatedUsing = OnRep_Nickname, BlueprintReadOnly, Category = "Lobby")
	//FString Nickname;

protected:
	UFUNCTION()
	void OnRep_IsReady();
	//UFUNCTION()
	//void OnRep_Nickname();

	UFUNCTION()
	void OnRep_CurrentColorIndex();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	bool IsColorAvailable(int32 CheckIndex);
};
