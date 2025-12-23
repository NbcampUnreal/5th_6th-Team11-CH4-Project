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

	if (FindSessionsButton)
	{
		FindSessionsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnFindSessionsClicked);
	}

	USessionSubsystem* Subsystem = GetSessionSubsystem();
	if (Subsystem)
	{
		Subsystem->OnSessionSearchUpdated.RemoveDynamic(this, &UMainMenuWidget::OnSessionSearchCompleted);
		Subsystem->OnSessionSearchUpdated.AddDynamic(this, &UMainMenuWidget::OnSessionSearchCompleted);

		Subsystem->OnJoinSessionFinished.RemoveDynamic(this, &UMainMenuWidget::OnJoinSessionCompleted);
		Subsystem->OnJoinSessionFinished.AddDynamic(this, &UMainMenuWidget::OnJoinSessionCompleted);
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

//void UMainMenuWidget::ApplyNickname()
//{
//	if (!NicknameInput) return;
//
//	FString InputName = NicknameInput->GetText().ToString();
//	if (InputName.IsEmpty())
//	{
//		InputName = TEXT("Player");
//	}
//
//	APlayerController* PC = GetOwningPlayer();
//	if (PC)
//	{
//
//		ALobbyPlayerState* MyPS = PC->GetPlayerState<ALobbyPlayerState>();
//		if (MyPS)
//		{
//			MyPS->SetNickname(InputName);
//		}
//	}
//
//	UE_LOG(LogTemp, Log, TEXT("[MainMenu] Nickname Applied: %s"), *InputName);
//}

void UMainMenuWidget::OnFindSessionsClicked()
{
	//ApplyNickname();

	USessionSubsystem* Subsystem = GetSessionSubsystem();
	if (Subsystem)
	{
		Subsystem->FindGameSessions(100, true);
	}
}

void UMainMenuWidget::OnSessionSearchCompleted(const TArray<FSessionInfo>& Sessions)
{
	if (!SessionListScrollBox || !SessionRowWidgetClass) return;

	SessionListScrollBox->ClearChildren();

	for (const FSessionInfo& Info : Sessions)
	{
		USessionRowWidget* RowWidget = CreateWidget<USessionRowWidget>(this, SessionRowWidgetClass);
		if (RowWidget)
		{
			RowWidget->Setup(Info, Info.SearchResultIndex);
			RowWidget->SetParentMenu(this);
			SessionListScrollBox->AddChild(RowWidget);
		}
	}

	if (Sessions.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("No sessions found."));
	}
}

void UMainMenuWidget::JoinServerAtIndex(int32 Index)
{
	USessionSubsystem* Subsystem = GetSessionSubsystem();
	if (Subsystem)
	{
		Subsystem->JoinFoundSession(Index);
	}
}

void UMainMenuWidget::OnJoinSessionCompleted(EJoinResultBP Result)
{
	switch (Result)
	{
	case EJoinResultBP::Success:
		UE_LOG(LogTemp, Log, TEXT("Joining Session..."));
		break;
	case EJoinResultBP::SessionIsFull:
		break;
	default:
		break;
	}
}
