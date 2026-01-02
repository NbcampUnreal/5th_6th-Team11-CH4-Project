// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenuWidget.h"
#include "UI/SessionRowWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "GameFramework/PlayerController.h"
#include "Session/LobbyPlayerState.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_CreateSession) Btn_CreateSession->OnClicked.AddDynamic(this, &UMainMenuWidget::OnCreateSessionClicked);
	if (Btn_FindSession)   Btn_FindSession->OnClicked.AddDynamic(this, &UMainMenuWidget::OnFindSessionClicked);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (GetSessionSubsystem())
		{
			GetSessionSubsystem()->OnSessionSearchUpdated.AddDynamic(this, &UMainMenuWidget::UpdateSessionList);
		}
	}
}

USessionSubsystem* UMainMenuWidget::GetSessionSubsystem() const
{
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		return GI->GetSubsystem<USessionSubsystem>();
	}
	return nullptr;
}

void UMainMenuWidget::OnCreateSessionClicked()
{
	if (!GetSessionSubsystem()) return;

	const FString& ServerIP = TEXT("127.0.0.1");

	GetSessionSubsystem()->CreateLobbyForDedicated(ServerIP, 7777, 6);
}

void UMainMenuWidget::OnFindSessionClicked()
{
	if (!GetSessionSubsystem()) return;

	if (SessionListScrollBox) SessionListScrollBox->ClearChildren();

	GetSessionSubsystem()->FindGameSessions(20, false);
}

void UMainMenuWidget::JoinServerAtIndex(int32 Index)
{
	USessionSubsystem* Subsystem = GetSessionSubsystem();
	if (Subsystem)
	{
		Subsystem->JoinFoundSession(Index);
	}
}

void UMainMenuWidget::UpdateSessionList(const TArray<FSessionInfo>& SessionInfos)
{
	if (!SessionListScrollBox || !SessionSlotClass) return;

	SessionListScrollBox->ClearChildren();

	for (const FSessionInfo& Info : SessionInfos)
	{
		UUserWidget* SlotWidget = CreateWidget<UUserWidget>(this, SessionSlotClass);
		if (USessionRowWidget* SessionSlot = Cast<USessionRowWidget>(SlotWidget))
		{
			SessionSlot->Setup(Info, Info.SearchResultIndex);
		}
		SessionListScrollBox->AddChild(SlotWidget);
	}
}