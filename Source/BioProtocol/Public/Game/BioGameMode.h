// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BioPlayerState.h"
#include "BioGameMode.generated.h"

class ABioGameState;
class ABioPlayerState;
class ABioPlayerController;
class AThirdSpectatorPawn;
class AIsolationDoor;

UCLASS()
class BIOPROTOCOL_API ABioGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ABioGameMode();

	virtual void StartPlay() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;


	void SetPlayerSpectating(AController* VictimController);

	// 게임 시작/종료
	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void EndGame();

	// 로딩 화면 숨기기
	void HideLoadingScreenFromAllPlayers();

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	

protected:

	float TimeCounter = 0.0f;

	UPROPERTY()
	ABioGameState* BioGameState;


	// 보이스 관련
	void CreateGameVoiceChannels();
	void CreatePublicGameChannel(const TArray<APlayerController*>& Players);
	void CreateMafiaGameChannel(const TArray<APlayerController*>& Players);
	void CreateGameChannel(EBioPlayerRole Team, const TArray<APlayerController*>& Players, const FString& ChannelName);

	void CreateLobbyVoiceChannel(const TArray<APlayerController*>& Players);

	void AddPlayerToExistingChannel(ABioPlayerController* BioPC, ABioPlayerState* PS, const FString& ChannelName);

	FString TrustedServerUrl = TEXT("http://localhost:3000");

	bool bGameVoiceStarted = false;  // 게임 시작 중복 방지
	bool bLobbyVoiceStarted = false;

	FString LobbyChannelName;
};