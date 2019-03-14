// Fill out your copyright notice in the Description page of Project Settings.

#include "LandMasterPlayerController.h"

void ALandMasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameAndUI());
}

