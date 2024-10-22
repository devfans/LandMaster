// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine.h"
#include "UnrealNetwork.h"
#include "LandMasterPawn.generated.h"


UCLASS(Blueprintable)
class ALandMasterPawn : public APawn
{
	GENERATED_BODY()

	/* The mesh component */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* ShipMeshComponent;

	/** The camera */
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComponent;

	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FPVCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(EditAnywhere, Category = WidgetComponent)
	class UWidgetComponent* WidgetComponent;

public:
  // ALandMasterPawn();
  ALandMasterPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  virtual void PostInitializeComponents() override;
  
  void CacheFireShootAction();

	/* Offset from the ships location to spawn projectiles */
	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite )
	FVector GunOffset;
	
	/* How fast the weapon will fire */
	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
	float FireRate;

	/* The speed our ship moves around the level */
	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
	float MoveSpeed;

	/* Sound to play each time we fire */
	UPROPERTY(Category = Audio, EditAnywhere, BlueprintReadWrite)
	class USoundBase* FireSound;

	// Begin Actor Interface
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End Actor Interface

	/* Fire a shot in the specified direction */
	UFUNCTION(Reliable, Server, WithValidation)
	void FireShot(FVector FireDirection);

	/* Handler for the fire timer expiry */
	void ShotTimerExpired();

	// Static names for axis bindings
	static const FName MoveForwardBinding;
	static const FName MoveRightBinding;
	static const FName FireForwardBinding;
	static const FName FireRightBinding;

	UFUNCTION(BlueprintCallable, Category = WidgetComponent)
		uint8 GetHP() { return CurrentHP; }

	UFUNCTION(BlueprintCallable, Category = WidgetComponent)
	void SetHP(uint8 hp);

	UFUNCTION(BlueprintCallable, Category = WidgetComponent)
	uint8 GetBullets() { return CurrentBullets; }

	UFUNCTION(BlueprintCallable, Category = WidgetComponent)
	void SetBullets(uint8 bullets);

	UPROPERTY()
	class UProgressBar* HPBar;

	UPROPERTY()
	class UProgressBar* BulletsBar;

	UFUNCTION(Reliable, NetMulticast, WithValidation)
	void UpdateBulletsBar(uint32 currentValue);
	UFUNCTION(Reliable, NetMulticast, WithValidation)
	void UpdateHPBar(uint32 currentValue);

	UFUNCTION()
		void OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	void CommitDamagePrivate(uint32 damage);

	void MoveTick(float DeltaSeconds);


private:

	/* Flag to control firing  */
	
	UPROPERTY(Replicated, EditAnywhere, Category = Player)
	uint32 bCanFire : 1;
	
	UPROPERTY(Replicated, EditAnywhere, Category = Player)
	uint32 bCanFireCache : 1;

	UPROPERTY(Replicated, EditAnywhere, Category = Player)
	FVector LastMoveDirection;

	/** Handle for efficient management of ShotTimerExpired timer */
	FTimerHandle TimerHandle_ShotTimerExpired;

	/* Player State */
	UPROPERTY(Replicated, EditAnywhere, Category = Player)
	uint8 CurrentHP;

	UPROPERTY(Replicated, EditAnywhere, Category = Player)
	uint8 CurrentBullets;

	const uint8 MaxHP = 100;
	const uint8 MaxBullets = 200;


public:
	/** Returns ShipMeshComponent subobject **/
	FORCEINLINE class UStaticMeshComponent* GetShipMeshComponent() const { return ShipMeshComponent; }
	/** Returns CameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
};

