// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VoiceChatResult.h"
#include "Net/VoiceConfig.h"
#include "BioPlayerController.generated.h"

class UBioPlayerHUD;
class IVoiceChatUser;

UCLASS()
class BIOPROTOCOL_API ABioPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABioPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;

public:
	// UI
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBioPlayerHUD> BioHUDClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UBioPlayerHUD* InGameHUD;

	// EOS Login
	UFUNCTION(BlueprintCallable, Category = "EOS")
	void LoginToEOS(int32 Credential);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateLobby(const FString& Ip, int32 Port, int32 PublicConnections);

	// Server RPCs
	UFUNCTION(Server, Reliable)
	void Server_SetEOSPlayerName(const FString& InEOSPlayerName);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartGameSession();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetReady();



	//  VoIP 말하기 제어
	UFUNCTION(BlueprintCallable, Category = "Voice|Native")
	void StartNativeVoIP();

	UFUNCTION(BlueprintCallable, Category = "Voice|Native")
	void StopNativeVoIP();


	void CreateVOIPTalker();


	// Voice Channel Management
	UFUNCTION(Client, Reliable)
	void Client_JoinLobbyChannel(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

	UFUNCTION(Client, Reliable) //Client_JoinMafiaChannel
	void Client_JoinGameChannel(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void LeaveLobbyChannel();

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void LeaveGameChannels(); //LeaveMafiaChannel

	// Voice Transmission Control
	UFUNCTION(BlueprintCallable, Category = "Voice")
	void VoiceTransmitToPublic();

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void VoiceTransmitToMafiaOnly();

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void VoiceTransmitToBothChannels();

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void VoiceTransmitToNone();



	// 음성 송신 제어 (VoIP)
	UFUNCTION(BlueprintCallable, Category = "Voice")
	void EnableMafiaVoice(bool bEnable);



	// Proximity Voice Settings
	UPROPERTY(EditDefaultsOnly, Category = "Voice|Proximity")
	float ProxUpdateInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Voice|Proximity")
	float ProxMinDist = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Voice|Proximity")
	float ProxMaxDist = 2000.f;

private:
	// EOS Login
	bool bLoginInProgress = false;
	FDelegateHandle LoginCompleteHandle;
	void HandleLoginComplete(int32, bool bOk, const FUniqueNetId& Id, const FString& Err);

	// Voice Chat
	IVoiceChatUser* VoiceChatUser = nullptr;
	void CacheVoiceChatUser();

	// Native VoIP - 추가
	UPROPERTY()
	UVOIPTalker* VOIPTalkerComponent = nullptr;  // ← 추가

	// Voice Channels
	FString LobbyChannelName;
	FString PublicGameChannelName;
	FString MafiaGameChannelName;

	void JoinLobbyChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);
	void JoinGameChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);
	void OnVoiceChannelJoined(const FString& ChannelName, const FVoiceChatResult& Result);

	// Proximity Voice
	FTimerHandle ProximityTimer;
	void StartProximityVoice();
	void UpdateProximityVoice();
	static float CalcProxVolume01(float Dist, float MinD, float MaxD);


	// (추가) 클라에서 PlayerArray 순회하여 리모트 VOIPTalker까지 보장
	FTimerHandle VOIPTalkerSyncTimer;

	void EnsureAllVOIPTalkers();
	void EnsureVOIPTalkerForPlayerState(APlayerState* InPlayerState);
	APawn* FindPawnForPlayerState(APlayerState* InPlayerState) const;

};