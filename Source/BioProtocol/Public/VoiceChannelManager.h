// VoiceChannelManager.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VoiceChannelManager.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Lobby,      // 로비 대기 중
    InGame,     // 게임 진행 중
    Returning   // 로비로 복귀 중
};

/**
 * 게임 페이즈별 보이스 채널 관리
 */
UCLASS()
class BIOPROTOCOL_API UVoiceChannelManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // 현재 게임 페이즈
    UPROPERTY(BlueprintReadOnly, Category = "Voice")
    EGamePhase CurrentPhase = EGamePhase::Lobby;

    // 채널 이름 저장
    FString LobbyChannelName;           // 로비 채널 (SessionSubsystem에서 자동 생성)
    FString PublicGameChannelName;      // 게임 내 전체 채널 (Trusted Server)
    FString MafiaGameChannelName;       // 게임 내 마피아 채널 (Trusted Server)

    // 로비 채널 재조인을 위해 credentials 캐싱
    FString LobbyClientBaseUrl;
    FString LobbyParticipantToken;
public:
    // 게임 시작 시 호출
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void OnGameStart();

    // 게임 종료 시 호출
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void OnGameEnd();

    // 로비 채널 이름 등록 (SessionSubsystem에서 호출)
    void RegisterLobbyChannel(const FString& ChannelName);

    // 현재 페이즈 확인
    UFUNCTION(BlueprintPure, Category = "Voice")
    EGamePhase GetCurrentPhase() const { return CurrentPhase; }

    // 채널 이름 조회
    UFUNCTION(BlueprintPure, Category = "Voice")
    FString GetLobbyChannelName() const { return LobbyChannelName; }

    UFUNCTION(BlueprintPure, Category = "Voice")
    FString GetPublicGameChannelName() const { return PublicGameChannelName; }

    UFUNCTION(BlueprintPure, Category = "Voice")
    FString GetMafiaGameChannelName() const { return MafiaGameChannelName; }

private:
    // 로비 맵 로드 완료 시점에 재조인
    void HandlePostLoadMap(UWorld* LoadedWorld);

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 추가
    void RegisterLobbyChannelCredentials(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

};