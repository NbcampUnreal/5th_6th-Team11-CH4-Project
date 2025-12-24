#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VoiceChatResult.h"
#include "TestController.generated.h"

class IVoiceChatUser;

UCLASS()
class BIOPROTOCOL_API ATestController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable)
    void LoginToEOS(int32 Credential);

    UFUNCTION(BlueprintCallable)
    void CreateLobby(const FString& Ip, int32 Port, int32 PublicConnections);

    UFUNCTION(Server, Reliable)
    void Server_SetEOSPlayerName(const FString& InEOSPlayerName);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_StartGameSession();

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_SetReady();

    // 채널 참가
    UFUNCTION(Client, Reliable)
    void Client_JoinLobbyChannel(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

    UFUNCTION(Client, Reliable)
    void Client_JoinGameChannel(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void LeaveLobbyChannel();

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void LeaveGameChannels();

    // 송신 제어
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void VoiceTransmitToPublic();

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void VoiceTransmitToMafiaOnly();

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void VoiceTransmitToBothChannels();

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void VoiceTransmitToNone();

private:
    bool bLoginInProgress = false;
    FDelegateHandle LoginCompleteHandle;

    void HandleLoginComplete(int32, bool bOk, const FUniqueNetId& Id, const FString& Err);
    void CacheVoiceChatUser();

    // 근접 보이스
    void StartProximityVoice();
    void UpdateProximityVoice();
    static float CalcProxVolume01(float Dist, float MinD, float MaxD);

    FTimerHandle ProximityTimer;
    float ProxUpdateInterval = 0.1f;
    float ProxMinDist = 300.f;
    float ProxMaxDist = 2000.f;

    IVoiceChatUser* VoiceChatUser = nullptr;

    // 채널 이름
    FString LobbyChannelName;
    FString PublicGameChannelName;
    FString MafiaGameChannelName;

    void JoinLobbyChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);
    void JoinGameChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);
    void OnVoiceChannelJoined(const FString& ChannelName, const FVoiceChatResult& Result);
};