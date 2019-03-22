// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LandMasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class LANDMASTER_API ALandMasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	UFUNCTION(BlueprintCallable, Category = "UMG Game")
	void SetShipPlayerName(const FString& InPlayerName);
	void SetDashInfo();

	UFUNCTION(Reliable, Client, WithValidation)
		void SetPlayerName();

	UFUNCTION(BlueprintCallable, Category = "UMG Game")
		void ChangeMenuWidget(TSubclassOf<UUserWidget> NewWidgetClass);

	UFUNCTION(BlueprintCallable, Category = "UMG Game")
		void Terminate();

protected:
	UPROPERTY()
		UUserWidget* CurrentWidget;

};
