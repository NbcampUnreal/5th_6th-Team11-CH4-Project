// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TestController.generated.h"


/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API ATestController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> MainMenuHUDClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> LobbyHUDClass;


public:
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartGameSession();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetReady();

	UFUNCTION(BlueprintCallable)
	void CreateLobby(const FString& Ip, int32 Port, int32 PublicConnections);


	void HandleLoginComplete(int32, bool bOk, const FUniqueNetId& Id, const FString& Err);

};
