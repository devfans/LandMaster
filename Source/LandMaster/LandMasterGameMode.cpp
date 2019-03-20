// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LandMasterGameMode.h"
#include "LandMasterPlayerController.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "ShipCharacter.h"

ALandMasterGameMode::ALandMasterGameMode()
{
	// set default pawn class to our character class
	DefaultPawnClass = AShipCharacter::StaticClass();
	PlayerControllerClass = ALandMasterPlayerController::StaticClass();

	DashInfo = "";
}


void ALandMasterGameMode::UpdateDashInfoText()
{
	if (nullptr != CurrentWidget)
	{
		UTextBlock* DashInfoText = Cast<UTextBlock>(CurrentWidget->GetWidgetFromName(TEXT("PlayModeText")));
		
		if (DashInfoText != nullptr)
		{
			FText info = FText::FromString(DashInfo);
			DashInfoText->SetText(info);
		}
	}
}

void ALandMasterGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpawnActors();
	ChangeMenuWidget(StartingWidgetClass);
	SetDashInfo();
}

void ALandMasterGameMode::SpawnActors()
{
	// UWorld* const World = GetWorld();
	// World->SpawnActor<ALandMasterPawn>();
}

void ALandMasterGameMode::ChangeMenuWidget(TSubclassOf<UUserWidget> NewWidgetClass)
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

void ALandMasterGameMode::SetServerAddress(FString &address)
{
	ServerAddress = address;
}

void ALandMasterGameMode::SetDashInfo()
{
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
	
	DashInfo = mode + GetWorld()->GetAddressURL();
	
	UpdateDashInfoText();
}


