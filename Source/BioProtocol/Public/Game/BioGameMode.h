// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BioPlayerState.h"
#include "BioGameMode.generated.h"

class ABioGameState;
class ABioPlayerState;
class AThirdSpectatorPawn;

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

	void SendPlayerToJail(AController* PlayerToJail);
	void CheckWinConditions();

	void SetPlayerSpectating(AController* VictimController);

	//biogamemode BP에서 BP_ThirdSpectatorPawn추가 필요
	UPROPERTY(EditDefaultsOnly, Category = "Spectator")
	TSubclassOf<AThirdSpectatorPawn> SpectatorPawnClass;



	// 게임 시작/종료
	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void EndGame();



protected:
	FVector JailLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Bio|GameRule")
	float DayDuration = 240.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bio|GameRule")
	int32 NightDuration = 45;

	float TimeCounter = 0.0f;
	void AssignRoles();
	void UpdateJailTimers(float DeltaTime);

	UPROPERTY()
	ABioGameState* BioGameState;


	// 보이스 관련
	void CreateGameVoiceChannels();
	void CreatePublicGameChannel(const TArray<APlayerController*>& Players);
	void CreateMafiaGameChannel(const TArray<APlayerController*>& Players);
	void CreateGameChannel(EBioPlayerRole Team, const TArray<APlayerController*>& Players, const FString& ChannelName);

	FString TrustedServerUrl = TEXT("http://localhost:3000");

	bool bGameVoiceStarted = false;  // 게임 시작 중복 방지


};