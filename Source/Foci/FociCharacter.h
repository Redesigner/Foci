// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "FociCharacter.generated.h"


UCLASS(config=Game)
class AFociCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* InteractTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	class UHitboxController* HitboxController;

	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;


	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PrimaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SecondaryAction;



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float AutoJumpVelocity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bInputEnabled = true;

	// Don't mark this as a UPROPERTY. This is a cast ref to movementcomponent
	class UMarleMovementComponent* MarleMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Targeting, meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AActor> FocusTarget;

public:
	AFociCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Primary(const FInputActionValue& Value);

	void Secondary(const FInputActionValue& Value);


	UFUNCTION(BlueprintImplementableEvent)
	void Attack();

	UFUNCTION(BlueprintImplementableEvent)
	void HitTarget(const FHitResult& HitResult);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	virtual bool CanJumpInternal_Implementation() const override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }



	virtual void OnWalkingOffLedge_Implementation(const FVector& PreviousFloorImpactNormal,
		const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta)  override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


	UFUNCTION(BlueprintCallable)
	void MoveToLocation(FVector Location);

	UFUNCTION(BlueprintImplementableEvent)
	void Mantle(FVector ResultingLocation);


	UFUNCTION(BlueprintCallable)
	void SetInputEnabled(bool bEnabled);


	UFUNCTION(BlueprintCallable)
	bool HasTarget() const;

	void LookAtTarget();

	void GrabLadder(class ALadder* Ladder);

	void Interact();

	UFUNCTION(BlueprintCallable)
	void SetFocusTarget(AActor* Target);

	UFUNCTION(BlueprintCallable)
	void ClearFocusTarget();

private:
	bool bMovingToLocation = false;

	FVector Destination;
};

