#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSessionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString SessionName;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PingInMs = 0;

    UPROPERTY(BlueprintReadOnly)
    FString ServerIp;

    UPROPERTY(BlueprintReadOnly)
    int32 ServerPort = 0;

    int32 SearchResultIndex = INDEX_NONE;
};

UENUM(BlueprintType)
enum class EJoinResultBP : uint8
{
    Success,
    SessionIsFull,
    SessionDoesNotExist,
    CouldNotRetrieveAddress,
    AlreadyInSession,
    UnknownError
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionSearchUpdated, const TArray<FSessionInfo>&, Sessions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinSessionFinished, EJoinResultBP, Result);

UCLASS()
class BIOPROTOCOL_API USessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Session")
    void CreateLobbyForDedicated(const FString& ServerIp, int32 ServerPort, int32 PublicConnections = 4);

    UFUNCTION(BlueprintCallable, Category = "Session")
    void FindGameSessions(int32 MaxResults = 100, bool bIsLAN = false);

    UFUNCTION(BlueprintCallable, Category = "Session")
    void JoinFoundSession(int32 index);

    UFUNCTION(BlueprintCallable, Category = "Session")
    void StartGameSession();

    UPROPERTY(BlueprintAssignable)
    FOnSessionSearchUpdated OnSessionSearchUpdated;

    UPROPERTY(BlueprintAssignable)
    FOnJoinSessionFinished OnJoinSessionFinished;

    UFUNCTION(BlueprintCallable)
    const TArray<FSessionInfo>& GetLastSessionInfos() const { return LastSessionInfos; }


protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    IOnlineSessionPtr GetSessionInterface() const;
    void TravelToDedicatedFromLobby(const FName& LobbySessionName);

    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
    TArray<FSessionInfo> LastSessionInfos;

    FDelegateHandle OnCreateSessionCompleteHandle;
    FDelegateHandle OnFindSessionsCompleteHandle;
    FDelegateHandle OnJoinSessionCompleteHandle;

    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};