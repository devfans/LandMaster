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
}


void ALandMasterGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpawnActors();
	ChangeMenuWidget(StartingWidgetClass);
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


void ALandMasterGameMode::PostLogin(APlayerController * NewPlayer)
{
	Super::PostLogin(NewPlayer);
	ALandMasterPlayerController* controller = Cast<ALandMasterPlayerController>(NewPlayer);
	if (controller != nullptr)
		controller->SetPlayerName();
}



