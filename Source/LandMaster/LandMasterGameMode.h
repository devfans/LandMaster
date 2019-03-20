// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "LandMasterGameMode.generated.h"

UCLASS(MinimalAPI)
class ALandMasterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALandMasterGameMode();

	UFUNCTION(BlueprintCallable, Category = "UMG Game")
	void ChangeMenuWidget(TSubclassOf<UUserWidget> NewWidgetClass);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UMG Game")
	FString ServerAddress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UMG Game")
		FString DashInfo;

	/*UPROPERTY()
		class UTextBlock* DashInfoText;*/


	UFUNCTION()
		void UpdateDashInfoText();

	UFUNCTION(BlueprintCallable, Category = "UMG Game")
	void SetServerAddress(FString &address);

	UFUNCTION(BlueprintCallable, Category = "UMG Game")
		void SetDashInfo();

	UFUNCTION(BlueprintCallable, Category = "UMG Game")
	void ConnectServer() {};

	// virtual bool ReadyToStartMatch_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = WidgetComponent)
		FString GetDashInfo() { return DashInfo; }

	void SpawnActors();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UMG Game")
	TSubclassOf<UUserWidget> StartingWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UMG Game")
		TSubclassOf<UUserWidget> InfoWidgetClass;


	UPROPERTY()
	UUserWidget* CurrentWidget;
};



