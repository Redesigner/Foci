// Copyright Epic Games, Inc. All Rights Reserved.

#include "FociCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Foci.h"

#include "Foci/Components/HitboxController.h"
#include "MarleMovementComponent.h"
#include "Ladder.h"
#include "Foci/Actors/Interactable.h"
#include "Foci/Components/WeaponTool.h"
#include "InteractableInterface.h"

//////////////////////////////////////////////////////////////////////////
// AFociCharacter

AFociCharacter::AFociCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMarleMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddUniqueDynamic(this, &AFociCharacter::OnBeginOverlap);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	MarleMovementComponent = Cast<UMarleMovementComponent>(GetCharacterMovement());

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(RootComponent);
	FirstPersonCamera->bUsePawnControlRotation = true;

	InteractTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("Interact Trigger"));
	InteractTrigger->SetupAttachment(RootComponent);

	HitboxController = CreateDefaultSubobject<UHitboxController>(TEXT("Hitbox Controller"));
	HitboxController->SetupAttachment(GetMesh());
	HitboxController->HitDetectedDelegate.BindUObject(this, &AFociCharacter::HitTarget);

	ViewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ViewMesh"));
	ViewMesh->SetupAttachment(FirstPersonCamera);
}

void AFociCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bMovingToLocation)
	{
		const FVector ActorLocation = GetActorLocation();
		if (FMath::IsNearlyEqual(ActorLocation.X, Destination.X, 10) && FMath::IsNearlyEqual(ActorLocation.Y, Destination.Y, 10))
		{
			bMovingToLocation = false;
			return;
		}
		AddMovementInput(Destination - GetActorLocation());
	}
	if (HasTarget())
	{
		LookAtTarget();
	}
}

void AFociCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	PlayerController = Cast<APlayerController>(NewController);
}

void AFociCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

bool AFociCharacter::CanJumpInternal_Implementation() const
{
	return true;
}

void AFociCharacter::OnWalkingOffLedge_Implementation(const FVector& PreviousFloorImpactNormal,
	const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta)
{
	const FVector Velocity = GetVelocity();
	const float VelocityXYSquared = Velocity.X * Velocity.X + Velocity.Y * Velocity.Y;
	const float AutoJumpVelocitySquared = AutoJumpVelocity * AutoJumpVelocity;

	if (VelocityXYSquared > AutoJumpVelocitySquared && CanJump())
	{
		Jump();
	}
}

void AFociCharacter::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA<ALadder>())
	{
		GrabLadder(Cast<ALadder>(OtherActor));
	}
}

void AFociCharacter::MoveToLocation(FVector Location)
{
	bMovingToLocation = true;
	Destination = Location;
}

void AFociCharacter::SetInputEnabled(bool bEnabled)
{
	bInputEnabled = bEnabled;
}

bool AFociCharacter::HasTarget() const
{
	return FocusTarget.IsValid();
}

void AFociCharacter::LookAtTarget()
{
	const FVector Difference = FocusTarget->GetActorLocation() - GetActorLocation();
	FRotator ActorRotation = GetActorRotation();

	const double OldYaw = ActorRotation.Yaw;
	const double NewYaw = FMath::RadiansToDegrees(FMath::Atan2(Difference.Y, Difference.X));
	const double YawDifference = FMath::UnwindDegrees(NewYaw - OldYaw);
	ActorRotation.Yaw = NewYaw;

	SetActorRotation(ActorRotation);

	FRotator CameraRotation = Controller->GetControlRotation();
	CameraRotation.Add(0.0, YawDifference, 0.0);
	Controller->SetControlRotation(CameraRotation);
}

void AFociCharacter::GrabLadder(ALadder* Ladder)
{
	MarleMovementComponent->GrabLadder(Ladder);
}

void AFociCharacter::Interact()
{
	TSet<AActor*> OverlappingActors;
	InteractTrigger->GetOverlappingActors(OverlappingActors);
	bool bFoundTarget = false;
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor->ActorHasTag(TEXT("Targetable")))
		{
			bFoundTarget = true;
			SetFocusTarget(Actor);
			continue;
		}
		if (!Actor->Implements<UInteractableInterface>())
		{
			continue;
		}
		IInteractableInterface::Execute_Interact(Actor, this);
	}
	if (!bFoundTarget)
	{
		ClearFocusTarget();
	}
}

void AFociCharacter::SetFocusTarget(AActor* Target)
{
	FocusTarget = Target;
	MarleMovementComponent->bOrientRotationToMovement = false;
	FRotator ActorRotation = GetActorRotation();

	const FVector Difference = FocusTarget->GetActorLocation() - GetActorLocation();
	const double NewYaw = FMath::RadiansToDegrees(FMath::Atan2(Difference.Y, Difference.X));
	ActorRotation.Yaw = NewYaw;
	SetActorRotation(ActorRotation);

	DisableFirstPerson(); 
	// ReleaseWeapon();
}

void AFociCharacter::ClearFocusTarget()
{
	FocusTarget = nullptr;
	MarleMovementComponent->bOrientRotationToMovement = true;
	if (bWeaponReady)
	{
		ReleaseWeapon();
	}
}

bool AFociCharacter::GetFirstPerson() const
{
	return bFirstPersonMode;
}

bool AFociCharacter::IsWeaponDrawn() const
{
	return bWeaponDrawn;
}

bool AFociCharacter::IsWeaponReady() const
{
	return bWeaponReady;
}

const FText& AFociCharacter::GetCurrentDialog() const
{
	return CurrentDialog;
}

void AFociCharacter::SetDialog(FText Dialog)
{
	CurrentDialog = Dialog;
}


void AFociCharacter::EnableFirstPerson()
{
	FirstPersonCamera->Activate();
	FollowCamera->Deactivate();
	bFirstPersonMode = true;
	MarleMovementComponent->bUseControllerDesiredRotation = true;
	MarleMovementComponent->bOrientRotationToMovement = false;
	GetMesh()->SetVisibility(false);
	ViewMesh->SetVisibility(true);

	if (CurrentWeapon)
	{
		CurrentWeapon->SetFirstPerson();
	}
}

void AFociCharacter::DisableFirstPerson()
{
	FollowCamera->Activate();
	FirstPersonCamera->Deactivate();
	bFirstPersonMode = false;
	MarleMovementComponent->bUseControllerDesiredRotation = false;
	if (!HasTarget())
	{
		MarleMovementComponent->bOrientRotationToMovement = true;
	}
	GetMesh()->SetVisibility(true);
	ViewMesh->SetVisibility(false);

	if (CurrentWeapon)
	{
		CurrentWeapon->SetThirdPerson();
	}
}



void AFociCharacter::SetFirstPerson(bool bFirstPerson)
{
	if (HasTarget())
	{
		return;
	}
	if (bFirstPerson && !bFirstPersonMode)
	{
		EnableFirstPerson();
		return;
	}
	if (!bFirstPerson && bFirstPersonMode)
	{
		DisableFirstPerson();
		return;
	}
}


//////////////////////////////////////////////////////////////////////////
// Input
void AFociCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		UE_LOG(LogTemp, Display, TEXT("Attaching PlayerInputComponent to %s"), *GetFName().ToString())

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFociCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFociCharacter::Look);

		//A-button equivalent
		EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AFociCharacter::Primary);

		//B-button equivalent
		EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Started, this, &AFociCharacter::Secondary);


		//Slot1
		EnhancedInputComponent->BindAction(Slot1Action, ETriggerEvent::Started, this, &AFociCharacter::Slot1Pressed);
		EnhancedInputComponent->BindAction(Slot1Action, ETriggerEvent::Completed, this, &AFociCharacter::Slot1Released);
	}

}

void AFociCharacter::Move(const FInputActionValue& Value)
{
	if (!bInputEnabled)
	{
		return;
	}
	if (!Controller)
	{
		return;
	}
	if (bMovingToLocation)
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MarleMovementComponent && MarleMovementComponent->UseDirectInput())
	{
		AddMovementInput(FVector::ForwardVector, MovementVector.Y);
		AddMovementInput(FVector::RightVector, MovementVector.X);
		return;
	}
	if (FocusTarget.IsValid())
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
		return;
	}
	// find out which way is forward
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void AFociCharacter::Look(const FInputActionValue& Value)
{
	if (!bInputEnabled)
	{
		return;
	}
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AFociCharacter::Primary(const FInputActionValue& Value)
{
	if (!bInputEnabled)
	{
		return;
	}
	if (MarleMovementComponent->MovementMode == EMovementMode::MOVE_Custom)
	{
		MarleMovementComponent->JumpAction();
		return;
	}
	Interact();
}

void AFociCharacter::Secondary(const FInputActionValue& Value)
{
	if (bFirstPersonMode)
	{
		DisableFirstPerson();
		if (bWeaponReady)
		{
			ReleaseWeapon();
		}
		return;
	}
	if (bWeaponReady)
	{
		ReleaseWeapon();
		return;
	}
	Attack();
}



void AFociCharacter::Slot1Pressed(const FInputActionValue& Value)
{
	// If we aren't targeting something, we want to enter first-person mode, first
	// third-person mode and using a slotted weapon/tool aren't compatible right now
	if (!HasTarget() && !bFirstPersonMode)
	{
		EnableFirstPerson();
		ReadyWeapon(Weapons[0]);
		return;
	}
	// we have a target, but our weapon isn't ready, so ready it
	if (!bWeaponReady)
	{
		ReadyWeapon(Weapons[0]);
	}
	if (!bWeaponDrawn)
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->Draw();
		}
		bWeaponDrawn = true;
	}
}

void AFociCharacter::Slot1Released(const FInputActionValue& Value)
{
	if (!bWeaponDrawn || !bWeaponReady)
	{
		return;
	}
	bWeaponDrawn = false;
	FireWeapon();
}

void AFociCharacter::ReadyWeapon(TSubclassOf<class AWeaponTool> Weapon)
{
	if (CurrentWeapon)
	{
		UE_LOG(LogWeaponSystem, Warning, TEXT("Attempted to ready a weapon before releasing the curent weapon."))
		return;
	}
	CurrentWeapon = Cast<AWeaponTool>(GetWorld()->SpawnActor(Weapon));
	CurrentWeapon->AttachComponentsToSockets(GetMesh(), ViewMesh, bFirstPersonMode, TEXT("Handle_R"));
	bWeaponReady = true;
}

void AFociCharacter::ReleaseWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy(true);
	}
	CurrentWeapon = nullptr;
	bWeaponReady = false;
}


void AFociCharacter::FireWeapon()
{
	if (!CurrentWeapon)
	{
		return;
	}
	CurrentWeapon->Fire(this, GetActorLocation() + FVector::UpVector * 35.0f,
		HasTarget() ? (FocusTarget->GetActorLocation() - GetActorLocation()).ToOrientationRotator() : GetControlRotation());
}