// TestController.h
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

public:
    // EOS 로그인
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

public:
    // ============ 채널 참가/나가기 ============

    // 로비 채널 참가 (SessionSubsystem에서 자동 호출)
    UFUNCTION(Client, Reliable)
    void Client_JoinLobbyChannel(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

    void JoinLobbyChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

    // 게임 채널 참가 (GameMode에서 호출)
    UFUNCTION(Client, Reliable)
    void Client_JoinGameChannel(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

    void JoinGameChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

    // 로비 채널 나가기
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void LeaveLobbyChannel();

    // 게임 채널 나가기
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void LeaveGameChannels();

public:
    // ============ 송신 제어 ============

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void VoiceTransmitToPublic();  // 전체 채널로 송신

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void VoiceTransmitToMafiaOnly();  // 마피아 채널로만 송신

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void VoiceTransmitToBothChannels();  // 두 채널 모두 송신

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void VoiceTransmitToNone();  // 송신 중지

    UFUNCTION(BlueprintCallable, Category = "Voice")
    void DebugVoiceStatus();
private:
    // 보이스 관련
    void StartProximityVoice();
    void UpdateProximityVoice();
    static float CalcProxVolume01(float Dist, float MinD, float MaxD);

    FTimerHandle ProximityTimer;
    float ProxUpdateInterval = 0.1f;
    float ProxMinDist = 300.f;
    float ProxMaxDist = 2000.f;

    IVoiceChatUser* VoiceChatUser = nullptr;
    void CacheVoiceChatUser();

    // ============ 채널 이름 저장 ============
    FString LobbyChannelName;         // 로비 채널
    FString PublicGameChannelName;    // 게임 내 전체 채널
    FString MafiaGameChannelName;     // 게임 내 마피아 채널

    void OnVoiceChannelJoined(const FString& ChannelName, const FVoiceChatResult& Result);
};