#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PawnMovementComponent.h"
#include "EnemyMovementComponent.generated.h"

UENUM(BlueprintType)
enum class EEnemyMovementMode : uint8
{
	MOVE_None,
	MOVE_Walking,
	MOVE_Falling
};

UCLASS()
class FOCI_API UEnemyMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;


	virtual void HandleBlockingImpact(FHitResult ImpactHitResult);

	virtual void PhysMovement(float DeltaTime);

	virtual void PhysFalling(float DeltaTime);

	virtual void PhysWalking(float DeltaTime);

	virtual void SetDefaultMovementMode();

	virtual bool FindFloor(FHitResult& OutHitResult) const;

	virtual bool SnapToFloor();

private:
	TWeakObjectPtr<class AEnemy> EnemyOwner;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = true))
	EEnemyMovementMode MovementMode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = true))
	float MaxSpeed = 300.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = true))
	float Friction = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = true))
	float Acceleration = 150.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Falling", meta = (AllowPrivateAccess = true))
	float FloorSnapDistance = 10.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Floor", meta = (AllowPrivateAccess = true, ClampMax = 90.0f, ClampMin = 0.0f))
	float MaxFloorWalkableAngle = 35.0f;
	float MaxFloorWalkableZ = 0.573576f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement|Floor", meta = (AllowPrivateAccess = true))
	TWeakObjectPtr<UPrimitiveComponent> Basis;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement|Floor", meta = (AllowPrivateAccess = true))
	FVector BasisNormal;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement|Floor", meta = (AllowPrivateAccess = true))
	FRotator BasisNormalRotator;


public:
	virtual void SetUpdatedComponent(USceneComponent* Component) override;

	bool IsFalling() const;
};