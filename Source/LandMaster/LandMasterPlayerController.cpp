// Fill out your copyright notice in the Description page of Project Settings.

#include "LandMasterPlayerController.h"
#include "ShipCharacter.h"

void ALandMasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameAndUI());
}

void ALandMasterPlayerController::SetShipPlayerName(const FString InPlayerName)
{
	
}