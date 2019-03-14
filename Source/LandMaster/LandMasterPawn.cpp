// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LandMasterPawn.h"
#include "LandMasterProjectile.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/ProgressBar.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

const FName ALandMasterPawn::MoveForwardBinding("MoveForward");
const FName ALandMasterPawn::MoveRightBinding("MoveRight");
const FName ALandMasterPawn::FireForwardBinding("FireForward");
const FName ALandMasterPawn::FireRightBinding("FireRight");



ALandMasterPawn::ALandMasterPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShipMesh(TEXT("/Game/TwinStick/Meshes/TwinStickUFO.TwinStickUFO"));
	// Create the mesh component
	ShipMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMeshComponent->SetSimulatePhysics(false);
	ShipMeshComponent->SetNotifyRigidBodyCollision(true);
	ShipMeshComponent->OnComponentHit.AddDynamic(this, &ALandMasterPawn::OnCompHit);
	ShipMeshComponent->BodyInstance.SetCollisionProfileName("OverlapOnlyPawn");

	RootComponent = ShipMeshComponent;
	// ShipMeshComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	// ShipMeshComponent->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	ShipMeshComponent->SetStaticMesh(ShipMesh.Object);

	// Cache our sound effect
	static ConstructorHelpers::FObjectFinder<USoundBase> FireAudio(TEXT("/Game/TwinStick/Audio/TwinStickFire.TwinStickFire"));
	FireSound = FireAudio.Object;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't want arm to rotate when ship does
	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->RelativeRotation = FRotator(-80.f, 0.f, 0.f);
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;	// Camera does not rotate relative to arm

	FPVCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FPVCamera"));
	FPVCameraComponent->SetupAttachment(RootComponent);
	FPVCameraComponent->bUsePawnControlRotation = true;


	// Movement
	MoveSpeed = 1000.0f;
	// Weapon
	GunOffset = FVector(90.f, 0.f, 0.f);
	FireRate = 0.3f;
	bCanFire = true;
	bCanFireCache = false;

	CurrentHP = 100;
	CurrentBullets = 200;

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	UClass* Widget = LoadClass<UUserWidget>(NULL, TEXT("WidgetBlueprint'/Game/Interface/HP.HP_C'"));
	WidgetComponent->SetWidgetClass(Widget);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	
	bReplicates = true;
	bReplicateMovement = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
}

void ALandMasterPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALandMasterPawn, CurrentHP);
	DOREPLIFETIME(ALandMasterPawn, CurrentBullets);
	DOREPLIFETIME(ALandMasterPawn, bCanFire);
	DOREPLIFETIME(ALandMasterPawn, bCanFireCache);
}

bool ALandMasterPawn::UpdateBulletsBar_Validate() { return true; }
void ALandMasterPawn::UpdateBulletsBar_Implementation()
{
	if (BulletsBar != nullptr)
		BulletsBar->SetPercent((float)CurrentBullets / (float)MaxBullets);
}

bool ALandMasterPawn::UpdateHPBar_Validate() { return true;  }
void ALandMasterPawn::UpdateHPBar_Implementation()
{
	if (HPBar != nullptr)
		HPBar->SetPercent((float)CurrentHP / (float)MaxHP);
}

void ALandMasterPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	WidgetComponent->InitWidget();
	UUserWidget* CurrentWidget = WidgetComponent->GetUserWidgetObject();
	if (nullptr != CurrentWidget)
	{
		HPBar = Cast<UProgressBar>(CurrentWidget->GetWidgetFromName(TEXT("HPProgressBar")));
		BulletsBar = Cast<UProgressBar>(CurrentWidget->GetWidgetFromName(TEXT("BulletsProgressBar")));
		UpdateHPBar();
		UpdateBulletsBar();
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("HP Widget object was not found!"));

	// AController* controller = GetController();
	// CameraComponent->SetActive(false);
	// FPVCameraComponent->SetActive(true);
}

void ALandMasterPawn::CacheFireShootAction() {
  UE_LOG(LogTemp, Warning, TEXT("Firing shoot action is caching!"));
  bCanFireCache = true;
}

void ALandMasterPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	// set up gameplay key bindings
	PlayerInputComponent->BindAxis(MoveForwardBinding);
	PlayerInputComponent->BindAxis(MoveRightBinding);
	PlayerInputComponent->BindAxis(FireForwardBinding);
	PlayerInputComponent->BindAxis(FireRightBinding);
	PlayerInputComponent->BindAction("FireShoot", IE_Pressed, this, &ALandMasterPawn::CacheFireShootAction);
}

void ALandMasterPawn::Tick(float DeltaSeconds)
{
	// Find movement direction
	const float ForwardValue = GetInputAxisValue(MoveForwardBinding);
	const float RightValue = GetInputAxisValue(MoveRightBinding);

	// Clamp max size so that (X=1, Y=1) doesn't cause faster movement in diagonal directions
	const FVector MoveDirection = FVector(ForwardValue, RightValue, 0.f).GetClampedToMaxSize(1.0f);
	

	// Calculate  movement
	const FVector Movement = MoveDirection * MoveSpeed * DeltaSeconds;

	// If non-zero size, move this actor
	if (Movement.SizeSquared() > 0.0f)
	{
		const FRotator NewRotation = Movement.Rotation();
		FHitResult Hit(1.f);
		RootComponent->MoveComponent(Movement, NewRotation, true, &Hit);
		LastMoveDirection = MoveDirection;
		
		if (Hit.IsValidBlockingHit())
		{
			const FVector Normal2D = Hit.Normal.GetSafeNormal2D();
			const FVector Deflection = FVector::VectorPlaneProject(Movement, Normal2D) * (1.f - Hit.Time);
			RootComponent->MoveComponent(Deflection, NewRotation, true);
		}
	}
	
	// Create fire direction vector
	// const float FireForwardValue = GetInputAxisValue(FireForwardBinding);
	// const float FireRightValue = GetInputAxisValue(FireRightBinding);
	// const FVector FireDirection = FVector(FireForwardValue, FireRightValue, 0.f);


	// Try and fire a shot
	if (bCanFire == true && bCanFireCache)
		UE_LOG(LogTemp, Warning, TEXT("Firing shoot action is caching......, direction: %s"), *MoveDirection.ToString());
	FireShot(LastMoveDirection);

	
}

bool ALandMasterPawn::FireShot_Validate(FVector FireDirection) { return true; }
void ALandMasterPawn::FireShot_Implementation(FVector FireDirection)
{
	// If it's ok to fire again
	if (bCanFire == true)
	{
		// If we are pressing fire stick in a direction
		// if (FireDirection.SizeSquared() > 0.0f)
    if (bCanFireCache == true)
		{
			const FRotator FireRotation = FireDirection.Rotation();
			// Spawn projectile at an offset from this pawn
			const FVector SpawnLocation = GetActorLocation() + FireRotation.RotateVector(GunOffset);

			UWorld* const World = GetWorld();
			if (World != NULL && CurrentBullets > 0)
			{
				// spawn the projectile
				World->SpawnActor<ALandMasterProjectile>(SpawnLocation, FireRotation);
				if (CurrentBullets > 0)
				{
					CurrentBullets--;
					UpdateBulletsBar();
				}
			}

			bCanFire = false;
			bCanFireCache = false;
			World->GetTimerManager().SetTimer(TimerHandle_ShotTimerExpired, this, &ALandMasterPawn::ShotTimerExpired, FireRate);

			// try and play the sound if specified
			if (FireSound != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
			}

			bCanFire = false;
		}
	}
}

void ALandMasterPawn::ShotTimerExpired()
{
	bCanFire = true;
}

void ALandMasterPawn::SetHP(uint8 hp)
{
	CurrentHP = hp;
}

void ALandMasterPawn::SetBullets(uint8 bullets)
{
	CurrentBullets = bullets;
}


void ALandMasterPawn::OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL))
	{
		if (CurrentHP > 0) {
			CurrentHP--;
			UpdateHPBar();
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Ship was damaged by %s"), *OtherActor->GetName());
}

void ALandMasterPawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL))
	{
		if (CurrentHP > 0) {
			CurrentHP--;
			UpdateHPBar();
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Ship was damaged by %s"), *OtherActor->GetName());

}

float ALandMasterPawn::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentHP > 8) {
		CurrentHP-=8;
		UpdateHPBar();
	}
	else {
		Destroy();
	}
	UE_LOG(LogTemp, Warning, TEXT("Ship was damaged by %s"), *DamageCauser->GetName());
	return 0.0f;
}

