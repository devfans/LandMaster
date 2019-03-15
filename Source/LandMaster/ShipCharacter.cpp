// Fill out your copyright notice in the Description page of Project Settings.

#include "ShipCharacter.h"

#include "LandMasterProjectile.h"
#include "ShipPlayerState.h"
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


AShipCharacter::AShipCharacter()
{

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShipMesh(TEXT("/Game/TwinStick/Meshes/TwinStickUFO.TwinStickUFO"));
	// Create the mesh component
	ShipMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMeshComponent->SetSimulatePhysics(false);
	ShipMeshComponent->SetNotifyRigidBodyCollision(true);
	ShipMeshComponent->SetCanEverAffectNavigation(false);
	ShipMeshComponent->SetGenerateOverlapEvents(false);
	ShipMeshComponent->OnComponentHit.AddDynamic(this, &AShipCharacter::OnCompHit);

	static FName MeshProfile(TEXT("CharacterMesh"));
	ShipMeshComponent->SetCollisionProfileName(MeshProfile);
	// ShipMeshComponent->BodyInstance.SetCollisionProfileName("OverlapOnlyPawn");
	RootComponent = ShipMeshComponent;
	// ShipMeshComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	// ShipMeshComponent->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	ShipMeshComponent->SetStaticMesh(ShipMesh.Object);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = 1000.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

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
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


	// bReplicates = true;
	// bReplicateMovement = true;
	// bReplicateInstigator = true;
	SetReplicates(true);
	SetReplicateMovement(true);


}

void AShipCharacter::MoveTick(FVector Direction, float Value)
{
	LastMoveDirection = Direction;
	AddMovementInput(Direction, Value);
	UE_LOG(LogTemp, Warning, TEXT("Moving direction: %s, %d"), *Direction.ToString(), Value);
}

void AShipCharacter::MoveForward(float Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Moving direction forward"));
	if ((Controller != NULL) && (Value != 0.f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotaion(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotaion).GetUnitAxis(EAxis::X);
		MoveTick(Direction, Value);
	}
}

void AShipCharacter::MoveRight(float Value)
{
	//if ((Controller != NULL) && (Value != 0.f))
	//{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotaion(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotaion).GetUnitAxis(EAxis::Y);
		MoveTick(Direction, Value);
	//}
}

void AShipCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShipCharacter, CurrentHP);
	DOREPLIFETIME(AShipCharacter, CurrentBullets);
	DOREPLIFETIME(AShipCharacter, bCanFire);
	DOREPLIFETIME(AShipCharacter, bCanFireCache);
	DOREPLIFETIME(AShipCharacter, LastMoveDirection);
}

bool AShipCharacter::UpdateBulletsBar_Validate(uint32 currentValue) { return true; }
void AShipCharacter::UpdateBulletsBar_Implementation(uint32 currentValue)
{
	if (BulletsBar != nullptr)
		BulletsBar->SetPercent((float)currentValue / (float)MaxBullets);
}

bool AShipCharacter::UpdateHPBar_Validate(uint32 currentValue) { return true; }
void AShipCharacter::UpdateHPBar_Implementation(uint32 currentValue)
{
	if (HPBar != nullptr)
		HPBar->SetPercent((float)currentValue / (float)MaxHP);
}

void AShipCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	WidgetComponent->InitWidget();
	UUserWidget* CurrentWidget = WidgetComponent->GetUserWidgetObject();
	if (nullptr != CurrentWidget)
	{
		HPBar = Cast<UProgressBar>(CurrentWidget->GetWidgetFromName(TEXT("HPProgressBar")));
		BulletsBar = Cast<UProgressBar>(CurrentWidget->GetWidgetFromName(TEXT("BulletsProgressBar")));
		UpdateHPBar(CurrentHP);
		UpdateBulletsBar(CurrentBullets);
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("HP Widget object was not found!"));

	// AController* controller = GetController();
	// CameraComponent->SetActive(false);
	// FPVCameraComponent->SetActive(true);
}

void AShipCharacter::CacheFireShootAction() {
	UE_LOG(LogTemp, Warning, TEXT("Firing shoot action is caching!"));
	bCanFireCache = true;
}

void AShipCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	// set up gameplay key bindings
	PlayerInputComponent->BindAxis("MoveForward", this, &AShipCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShipCharacter::MoveRight);

	PlayerInputComponent->BindAction("FireShoot", IE_Pressed, this, &AShipCharacter::CacheFireShootAction);
}

void AShipCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FireShot(LastMoveDirection);
}

bool AShipCharacter::FireShot_Validate(FVector FireDirection) { return true; }
void AShipCharacter::FireShot_Implementation(FVector FireDirection)
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
					UpdateBulletsBar(CurrentBullets);
				}
			}

			bCanFire = false;
			bCanFireCache = false;
			World->GetTimerManager().SetTimer(TimerHandle_ShotTimerExpired, this, &AShipCharacter::ShotTimerExpired, FireRate);

			// try and play the sound if specified
			if (FireSound != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
			}

			bCanFire = false;
		}
	}
}

void AShipCharacter::ShotTimerExpired()
{
	bCanFire = true;
}

void AShipCharacter::SetHP(uint8 hp)
{
	CurrentHP = hp;
}

void AShipCharacter::SetBullets(uint8 bullets)
{
	CurrentBullets = bullets;
}


void AShipCharacter::OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL))
	{
	}
	UE_LOG(LogTemp, Warning, TEXT("Ship was damaged by %s"), *OtherActor->GetName());
}

void AShipCharacter::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL))
	{

	}
	UE_LOG(LogTemp, Warning, TEXT("Ship was damaged by %s"), *OtherActor->GetName());

}

float AShipCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Role == ROLE_Authority)
		CommitDamagePrivate(8);

	UE_LOG(LogTemp, Warning, TEXT("Ship was damaged by %s"), *DamageCauser->GetName());
	return 0.0f;
}

void AShipCharacter::CommitDamagePrivate(uint32 damage)
{
	if (CurrentHP > damage) {
		CurrentHP -= damage;
		UpdateHPBar(CurrentHP);
	}

}

