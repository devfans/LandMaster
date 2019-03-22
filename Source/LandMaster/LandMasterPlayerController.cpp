// Fill out your copyright notice in the Description page of Project Settings.

#include "LandMasterPlayerController.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "ShipCharacter.h"
#include "MainGameInstance.h"


void ALandMasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameAndUI());
	SetDashInfo();
}

void ALandMasterPlayerController::SetShipPlayerName(const FString& InPlayerName)
{

	AShipCharacter* ship = Cast<AShipCharacter>(GetPawn());
	if (ship != nullptr) {
		ship->ServerSetName(InPlayerName);
		UE_LOG(LogTemp, Warning, TEXT("Setting new ship name to %s!"), *InPlayerName);
	} else
		UE_LOG(LogTemp, Warning, TEXT("Setting new ship name failed for new pawn yet!"));
	
}

bool ALandMasterPlayerController::SetPlayerName_Validate() { return true; }
void ALandMasterPlayerController::SetPlayerName_Implementation() {
	UMainGameInstance * instance = Cast<UMainGameInstance>(GetGameInstance());
	UE_LOG(LogTemp, Warning, TEXT("Setting new ship name from instance to %s!"), *instance->PlayerName);
	
	SetShipPlayerName(instance->PlayerName);
}

void ALandMasterPlayerController::SetDashInfo()
{
	UClass* Widget = LoadClass<UUserWidget>(NULL, TEXT("WidgetBlueprint'/Game/TwinStickCPP/Maps/InfoDash.InfoDash_C'"));
	ChangeMenuWidget(Widget);
	FString mode = "";
	switch (GetNetMode())
	{
	case NM_ListenServer:
		mode = "Host mode:";
		break;
	case NM_Client:
		mode = "Client connected to: ";
		break;
	case NM_DedicatedServer:
		mode = "Dedicated Server:";
		break;

	}

	if (nullptr != CurrentWidget)
	{
		UTextBlock* DashInfoText = Cast<UTextBlock>(CurrentWidget->GetWidgetFromName(TEXT("PlayModeText")));

		if (DashInfoText != nullptr)
		{
			FText info = FText::FromString(mode + GetWorld()->GetAddressURL());
			DashInfoText->SetText(info);
		}
	}
}


void ALandMasterPlayerController::ChangeMenuWidget(TSubclassOf<UUserWidget> NewWidgetClass)
{
	if (CurrentWidget != nullptr)
	{
		CurrentWidget->RemoveFromViewport();
		CurrentWidget = nullptr;
	}
	if (NewWidgetClass != nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), NewWidgetClass);
		if (CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
		}
	}
}

void ALandMasterPlayerController::Terminate()
{
	if (nullptr != CurrentWidget)
	{
		UTextBlock* EndTextEntry = Cast<UTextBlock>(CurrentWidget->GetWidgetFromName(TEXT("EndTextEntry")));

		if (EndTextEntry != nullptr)
			EndTextEntry->SetVisibility(ESlateVisibility::Visible);

		
		if (GetNetMode() == ENetMode::NM_Client)
		{
			UButton* QuitButton = Cast<UButton>(CurrentWidget->GetWidgetFromName(TEXT("QuitButton")));
			UTextBlock* QuitButtonText = Cast<UTextBlock>(CurrentWidget->GetWidgetFromName(TEXT("QuitButtonText")));
			if (QuitButton != nullptr)
				QuitButton->SetVisibility(ESlateVisibility::Visible);

			if (QuitButtonText != nullptr)
				QuitButtonText->SetVisibility(ESlateVisibility::Visible);

		}
		
	}
}
