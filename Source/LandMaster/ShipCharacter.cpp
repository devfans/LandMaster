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
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "LandMasterPlayerController.h"
#include "MainGameInstance.h"
#include "DrawDebugHelpers.h"



AShipCharacter::AShipCharacter()
{

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShipMesh(TEXT("/Game/TwinStick/Meshes/TwinStickUFO.TwinStickUFO"));
	// Create the mesh component
	ShipMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMeshComponent->SetSimulatePhysics(false);
	// ShipMeshComponent->SetNotifyRigidBodyCollision(true);
	ShipMeshComponent->SetCanEverAffectNavigation(true);
	ShipMeshComponent->SetGenerateOverlapEvents(true);
	ShipMeshComponent->OnComponentHit.AddDynamic(this, &AShipCharacter::OnCompHit);

	static FName MeshProfile(TEXT("CharacterMesh"));
	ShipMeshComponent->SetCollisionProfileName(MeshProfile);
	ShipMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	// ShipMeshComponent->BodyInstance.SetCollisionProfileName("OverlapOnlyPawn");
	// RootComponent = ShipMeshComponent;
	ShipMeshComponent->SetupAttachment(GetMesh());
	// ShipMeshComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	// ShipMeshComponent->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	ShipMeshComponent->SetStaticMesh(ShipMesh.Object);
	ShipMeshComponent->bAbsoluteLocation = false;
	ShipMeshComponent->bAbsoluteRotation = false;
	// ShipMeshComponent->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> Beam(TEXT("ParticleSystem'/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/ICE/P_Beam_Laser_Ice_Large.P_Beam_Laser_Ice_Large'"));
	// UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Beam.Object, GetActorLocation()); 
	LaserEmitter = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LaserBeamEmitter"));
	LaserEmitter->SetTemplate(Beam.Object);
	LaserEmitter->SetupAttachment(GetMesh());
	// LaserEmitter->SetBeamSourcePoint(0, GetActorLocation(), 0);
	// LaserEmitter->SetBeamTargetPoint(0, GetActorLocation(), 0);
	LaserEmitter->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> BeamEye(TEXT("ParticleSystem'/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/ICE/P_Beam_Laser_Ice.P_Beam_Laser_Ice'"));
    
	// UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Beam.Object, GetActorLocation());
	LaserEyeEmitter = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LaserBeamEyeEmitter"));
	LaserEyeEmitter->SetTemplate(BeamEye.Object);
	LaserEyeEmitter->SetupAttachment(GetMesh());
	
	LaserEyeEmitter->SetVisibility(false);


	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->DefaultLandMovementMode = EMovementMode::MOVE_Flying;
	GetCharacterMovement()->AirControl = 0.1f;
	GetCharacterMovement()->MaxWalkSpeed = 5.f;

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
	FPVCameraComponent->bAbsoluteLocation = false;
	FPVCameraComponent->bAbsoluteRotation = false;
	FVector FPVCameraLocation(-240.f, 0.f, 160.f);
	FPVCameraComponent->SetRelativeLocation(FPVCameraLocation);
	FPVCameraComponent->RelativeRotation = FRotator(-15.f, 0.f, 0.f);
	FPVCameraComponent->SetupAttachment(RootComponent);
	FPVCameraComponent->bUsePawnControlRotation = false;

	static ConstructorHelpers::FObjectFinder<UParticleSystem> BeamShotTemplate(TEXT("ParticleSystem'/Game/InfinityBladeEffects/Effects/FX_Mobile/ICE/combat/P_ProjectileLob_Explo_Ice_02.P_ProjectileLob_Explo_Ice_02'"));
	BeamShot = BeamShotTemplate.Object;

	// Movement
	MoveSpeed = 1000.0f;

	// Weapon
	GunOffset = FVector(90.f, 0.f, 0.f);
	FireRate = 0.3f;
	bCanFire = true;
	bCanFireCache = false;
	bCanTraceCache = false;
	bHasName = false;

	CurrentHP = 100;
	CurrentBullets = 200;
	PlayerName = "";

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	UClass* Widget = LoadClass<UUserWidget>(NULL, TEXT("WidgetBlueprint'/Game/Interface/HP.HP_C'"));
	WidgetComponent->SetWidgetClass(Widget);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


	bReplicates = true;
	bReplicateMovement = true;
	bFPVMode = false;
	// bReplicateInstigator = true;

	bUseControllerRotationPitch = true;	
	
	CameraComponent->bAutoActivate = false;

}

void AShipCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShipCharacter, CurrentHP);
	DOREPLIFETIME(AShipCharacter, CurrentBullets);
	DOREPLIFETIME(AShipCharacter, PlayerName);

}

bool AShipCharacter::UpdateBulletsBar_Validate(uint32 currentValue) { return true; }
void AShipCharacter::UpdateBulletsBar_Implementation(uint32 currentValue)
{
	if (BulletsBar != nullptr)
		BulletsBar->SetPercent((float)currentValue / (float)MaxBullets);
}

bool AShipCharacter::UpdatePlayerName_Validate(const FString& InPlayerName) { return true; }
void AShipCharacter::UpdatePlayerName_Implementation(const FString& InPlayerName)
{
	if (PlayerNameText != nullptr)
	{
		FText name = FText::FromString(InPlayerName);
		PlayerNameText->SetText(name);
	}
	
}

bool AShipCharacter::UpdateHPBar_Validate(uint32 currentValue) { return true; }
void AShipCharacter::UpdateHPBar_Implementation(uint32 currentValue)
{
	if (HPBar != nullptr)
		HPBar->SetPercent((float)currentValue / (float)MaxHP);
}

void AShipCharacter::OnRep_SetHP()
{
	UpdateHPDisplay();
}

void AShipCharacter::UpdateHPDisplay()
{
	if (HPBar != nullptr)
		HPBar->SetPercent((float)CurrentHP / (float)MaxHP);
}

void AShipCharacter::OnRep_SetBullets()
{
	UpdateBulletsDisplay();
}

void AShipCharacter::UpdateBulletsDisplay()
{
	if (BulletsBar != nullptr)
		BulletsBar->SetPercent((float)CurrentBullets / (float)MaxBullets);
}

void AShipCharacter::OnRep_SetName()
{
	UpdateNameDisplay();
}

void AShipCharacter::UpdateNameDisplay()
{
	if (PlayerNameText != nullptr)
	{
		FText name = FText::FromString(PlayerName);
		PlayerNameText->SetText(name);
	}
}

void AShipCharacter::ClientSetName_Implementation()
{
	UMainGameInstance * instance = Cast<UMainGameInstance>(GetGameInstance());
	ServerSetName(instance->PlayerName);
}

bool AShipCharacter::ServerSetName_Validate(const FString& InName) { return true; }
void AShipCharacter::ServerSetName_Implementation(const FString& InName)
{
	bHasName = true;
	PlayerName = InName;
	UE_LOG(LogTemp, Warning, TEXT("Setting ship name to %s!"), *InName);
	if (Role == ROLE_Authority)
		UpdateNameDisplay();
}


void AShipCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (bHasName == false)
		ClientSetName();
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
		PlayerNameText = Cast<UTextBlock>(CurrentWidget->GetWidgetFromName(TEXT("PlayerNameText")));
		UpdateHPBar(CurrentHP);
		UpdateBulletsBar(CurrentBullets);
		// UpdatePlayerName(PlayerName);
		// ClientSetName();	
		// SetPlayerName(instance->PlayerName);
		

	}
	else
		UE_LOG(LogTemp, Warning, TEXT("HP Widget object was not found!"));

	// AController* controller = GetController();
	// CameraComponent->SetActive(false);
	// FPVCameraComponent->SetActive(true);
}

void AShipCharacter::CacheFireShootAction() {
	// UE_LOG(LogTemp, Warning, TEXT("Firing shoot action is caching!"));
	bCanFireCache = true;
}

void AShipCharacter::CacheFireLaserAction() {
	// UE_LOG(LogTemp, Warning, TEXT("Firing shoot action is caching!"));
	bCanTraceCache = true;
}

bool AShipCharacter::ServerRotateShip_Validate(float Value) { return true;  }
void AShipCharacter::ServerRotateShip_Implementation(float Value)
{	
	NRotateShip(Value);
}

void AShipCharacter::RotateAction(float Value)
{
	if (Value != 0.f)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Rotating Ship with delta %f"), Value);
		//FRotator rot(0.f, 20*Value, 0.f);
		//AddActorLocalRotation(rot);
		AddControllerYawInput(Value);
	}
}

void AShipCharacter::GoUpAction(float Value)
{
	if (Value != 0.f)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Rotating Ship with delta %f"), Value);
		//FRotator rot(0.f, 20*Value, 0.f);
		//AddActorLocalRotation(rot);
		AddControllerPitchInput(Value);
	}
}

bool AShipCharacter::NRotateShip_Validate(float Value) { return true; }
void AShipCharacter::NRotateShip_Implementation(float Value)
{
	RotateAction(Value);
}

void AShipCharacter::SwitchView()
{
	bFPVMode = !bFPVMode;
	if (bFPVMode == true)
	{
		FPVCameraComponent->SetActive(true);
		CameraComponent->SetActive(false);
	} 
	else
	{
		FPVCameraComponent->SetActive(false);
		CameraComponent->SetActive(true);
	}
}

void AShipCharacter::MoveForward(float Value)
{
	if (Controller != NULL && Value != 0.f)
	{
		// const FRotator Rotation = Controller->GetControlRotation();
		// const FRotator YawRotation(0.f, Rotation.Yaw, 0);

		//const FVector Direction = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);
		// const FVector dir(0.f, 0.f, 100);
		AddMovementInput(GetActorForwardVector(), Value, true);
		// SetActorRotation(Direction.Rotation());
		// FRotator rot(0.f, 90.f, 0.f);
		// AddActorLocalRotation(rot);
	}
}

void AShipCharacter::MoveRight(float Value)
{
	if (Controller != NULL && Value != 0.f)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);

	}
}

void AShipCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AShipCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShipCharacter::MoveRight);
	PlayerInputComponent->BindAxis("RotateShip", this, &AShipCharacter::RotateAction);
	PlayerInputComponent->BindAxis("GoUp", this, &AShipCharacter::GoUpAction);
	PlayerInputComponent->BindAction("FireShoot", IE_Pressed, this, &AShipCharacter::CacheFireShootAction);
	PlayerInputComponent->BindAction("SwitchView", IE_Pressed, this, &AShipCharacter::SwitchView);
	PlayerInputComponent->BindAction("FireLaser", IE_Pressed, this, &AShipCharacter::CacheFireLaserAction);
}

void AShipCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	LaserEyeEmitter->SetBeamSourcePoint(0, GetActorLocation(), 0);
	LaserEmitter->SetBeamSourcePoint(0, GetActorLocation(), 0);

	// Find movement direction
	// Clamp max size so that (X=1, Y=1) doesn't cause faster movement in diagonal directions

	// FireShot(GetActorForwardVector());
	if (bCanFireCache || bCanTraceCache) {
		const FRotator FireRotation = GetActorForwardVector().Rotation();
		// Spawn projectile at an offset from this pawn
		const FVector SpawnLocation = GetActorLocation() + FireRotation.RotateVector(GunOffset);
		if (bCanFireCache)
		{
			EmitBullet(FireRotation, SpawnLocation);
			bCanFireCache = false;
		}
		else 
		{
			EmitLaser(SpawnLocation);
			bCanTraceCache = false;
		}
		

		if (FireSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		}

	}
	//FVector Direction = GetActorForwardVector();
	//UWorld* const World = GetWorld();

	//FHitResult OutHit;
	//FVector Target = (Direction * 100000000.f) + GetActorLocation();
	//FVector End = Target;
	//FCollisionQueryParams CollisionParams;
	//// DrawDebugLine(World, Location, Target, FColor::Red, true);
	//// AShipCharacter::EmitLaserEffect(Location, Target);
	//if (World->LineTraceSingleByChannel(OutHit, GetActorLocation(), Target, ECC_Visibility, CollisionParams))
	//{
	//	if (OutHit.bBlockingHit)
	//	{
	//		AActor * other = OutHit.GetActor();
	//		// LaserEmitter->SetBeamTargetPoint(0, other->GetActorLocation(), 0);
	//		End = OutHit.Location;
	//	}
	//}
	//LaserEyeEmitter->SetBeamTargetPoint(0, End, 0);
}

bool AShipCharacter::FireShot_Validate(FVector FireDirection) { return true; }
void AShipCharacter::FireShot_Implementation(FVector FireDirection) 
{
	FireShotAction(FireDirection);
}

bool AShipCharacter::EmitBullet_Validate(FRotator Rotation, FVector Location) { return true;  }
void AShipCharacter::EmitBullet_Implementation(FRotator Rotation, FVector Location)
{
	UWorld* const World = GetWorld();
	if (World != NULL && CurrentBullets > 0)
	{
		// spawn the projectile
		ALandMasterProjectile * bullet = World->SpawnActor<ALandMasterProjectile>(Location, Rotation);
		bullet->SetOwner(this);
		{
			CurrentBullets--;
			if (Role == ROLE_Authority)
				UpdateBulletsDisplay();
		}
	}
}
bool AShipCharacter::EmitLaserEffect_Validate(FVector Start, FVector End) { return true; }
void AShipCharacter::EmitLaserEffect_Implementation(FVector Start, FVector End)
{
	// DrawDebugLine(GetWorld(), Start, End, FColor::Red, true);
	LaserEmitter->SetBeamTargetPoint(0, End, 0);
	LaserEmitter->SetVisibility(true);
	GetWorld()->GetTimerManager().SetTimer(BeamTimer, this, &AShipCharacter::LaserElapse, 0.1f);
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamShot, End);
}

void AShipCharacter::LaserElapse()
{
	LaserEmitter->SetVisibility(false);
}
bool AShipCharacter::EmitLaser_Validate(FVector Location) { return true; }
void AShipCharacter::EmitLaser_Implementation(FVector Location)
{
	FVector Direction = GetActorForwardVector();
	UWorld* const World = GetWorld();
	
	FHitResult OutHit;
	FVector Target = (Direction * 100000000.f) + Location;
	FVector End = Target;
	FCollisionQueryParams CollisionParams;
	// DrawDebugLine(World, Location, Target, FColor::Red, true);
	// AShipCharacter::EmitLaserEffect(Location, Target);
	if (World->LineTraceSingleByChannel(OutHit, Location, Target, ECC_Visibility, CollisionParams))
	{
		if (OutHit.bBlockingHit)
		{
			AActor * other = OutHit.GetActor();
			// LaserEmitter->SetBeamTargetPoint(0, other->GetActorLocation(), 0);
			End = OutHit.Location;
			UPrimitiveComponent* DamagedComponent = OutHit.GetComponent();
			if (DamagedComponent != nullptr)
			{
				// UE_LOG(LogTemp, Warning, TEXT("Laser hitting %s"), *other->GetName());
				DamagedComponent->AddImpulseAtLocation(GetActorForwardVector()*500000.0f, OutHit.Location);
			}
				
			AShipCharacter * otherShip = Cast<AShipCharacter>(other);
			FString targetName = otherShip == nullptr ? other->GetName() : otherShip->PlayerName;
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("%s hits %s"), *PlayerName, *targetName));
			if (other != this)
				UGameplayStatics::ApplyDamage(OutHit.GetActor(), 20.0f, nullptr, this, UDamageType::StaticClass());
		}
	}

	AShipCharacter::EmitLaserEffect(Location, End);
}

void AShipCharacter::FireShotAction(FVector FireDirection)
{
	// UE_LOG(LogTemp, Warning, TEXT("FireShot, can %b cached %b"), bCanFire, bCanFireCache);
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
				// EmitBullet(FireRotation, SpawnLocation);
				// projectile->SetOwner(this);
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

void AShipCharacter::SetPlayerName(const FString& InPlayerName)
{
	if (IsLocallyControlled())
		UpdatePlayerName(InPlayerName);
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
		CommitDamagePrivate(Damage);

	UE_LOG(LogTemp, Warning, TEXT("Ship was damaged by %s"), *DamageCauser->GetName());
	return 0.0f;
}

void AShipCharacter::CommitDamagePrivate(uint32 damage)
{
	if (CurrentHP > damage) 
	{
		CurrentHP -= damage;
		if (Role == ROLE_Authority)
			UpdateHPDisplay();
		// UpdateHPBar(CurrentHP);
	}
	else
	{
		CurrentHP = 0;
		if (Role == ROLE_Authority)
			UpdateHPDisplay();

		// UpdateHPBar(CurrentHP);
		Terminate();
		Destroy();
	}
}

void AShipCharacter::Terminate_Implementation()
{
	if (IsLocallyControlled())
	{
		ALandMasterPlayerController *controller = Cast<ALandMasterPlayerController>(GetController());
		if (controller != nullptr)
			controller->Terminate();
	}
}

