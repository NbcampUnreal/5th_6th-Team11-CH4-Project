// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TestController.generated.h"


class IVoiceChatUser;

UCLASS()
class BIOPROTOCOL_API ATestController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:


public:
	// EOS
	UFUNCTION(BlueprintCallable)
	void LoginToEOS(int32 Credential);

	UFUNCTION(BlueprintCallable)
	void CreateLobby(const FString& Ip, int32 Port, int32 PublicConnections);

	bool bLoginInProgress = false;
	FDelegateHandle LoginCompleteHandle;

	void HandleLoginComplete(int32, bool bOk, const FUniqueNetId& Id, const FString& Err);

	UFUNCTION(Server, Reliable)
	void Server_SetEOSPlayerName(const FString& InEOSPlayerName);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartGameSession();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetReady();


	// 보이스 채널

	UFUNCTION(Client, Reliable)
	void Client_JoinPrivateVoiceChannel(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void JoinPrivateVoiceChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

	// PTT용(블루프린트에서 눌렀다/뗐다 연결해도 됨)
	UFUNCTION(BlueprintCallable, Category = "Voice")
	void VoiceTransmitToAll();

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void VoiceTransmitToChannel(const FString& ChannelName);

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void VoiceTransmitToNone();

private:
	// 보이스 관련
	void StartProximityVoice();
	void UpdateProximityVoice();

	static float CalcProxVolume01(float Dist, float MinD, float MaxD);

	FTimerHandle ProximityTimer;

	float ProxUpdateInterval = 0.1f;
	float ProxMinDist = 300.f;	// 볼륨이 작아지기 시작하는 거리
	float ProxMaxDist = 2000.f;	// 최소 볼륨이 되는 거리


	IVoiceChatUser* VoiceChatUser = nullptr;
	void CacheVoiceChatUser();


	FString PublicChannelName;   // 로비 채널 이름 캐시
	FString PrivateChannelName;

};
