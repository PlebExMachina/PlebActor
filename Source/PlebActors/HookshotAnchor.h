// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HookshotAnchor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnchorCollision, class AHookshotAnchor* , Anchor, FHitResult, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnchorExpire,    class AHookshotAnchor* , Anchor, FTransform, ExpireTransform);

UCLASS()
class PLEBACTORS_API AHookshotAnchor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Controls the projectile movement of the Actor. Can be modified in BP if odd behavior is desired.
	UPROPERTY(EditAnywhere)
	class UProjectileMovementComponent* ProjectileMovement;

	// The collider for the Anchor (also the root)
	UPROPERTY(EditAnywhere)
	class USphereComponent* Collider;

	// Arrow (hidden in game) for debugging purposes.
	UPROPERTY(EditAnywhere)
	class UArrowComponent* Arrow;

	// Called when the hookshot anchor collides with a surface.
	UPROPERTY(BlueprintAssignable)
	FOnAnchorCollision OnHookshotHit;

	/*
		Called when the 'pulled actor' collides with a surface.
		This can also be used to determine when the pull is finished.
	*/
	UPROPERTY(BlueprintAssignable)
	FOnAnchorCollision OnPullBlocked;

	// Called when the anchor expires (max distance reached.)
	UPROPERTY(BlueprintAssignable)
	FOnAnchorExpire OnAnchorExpire;

	// How quickly the actor will be pulled to the target location.
	UPROPERTY(EditAnywhere)
	float PullSpeed;

	// The maximum distance the anchor can stray from it's original position until it's destroyed.
	UPROPERTY(EditAnywhere)
	float MaxDistance;

	// The amount of time before 'collision turns on.' Provides a small buffer so it doesn't collide with the actor that spawns it.
	UPROPERTY(EditAnywhere)
	float TimeUntilLive;

	// How close to the anchor location to 'cut off' the connection.
	UPROPERTY(Editanywhere)
	float CutoffDistance;

protected:
	// The actor being pulled by the anchor.
	UPROPERTY(Replicated)
	AActor* _PullActor;

	// Whether or not the projectile is 'live' or not. If end user wants it to be life by default they can set it.
	UPROPERTY(EditAnywhere, Replicated, ReplicatedUsing="OnRep_UpdateCollision")
	bool _IsLive;

	// The initial spawn location of the actor.
	UPROPERTY()
	FVector _SpawnLocation;

	// A replicated variable to allow for dynamic projectile speed.
	UPROPERTY(Replicated, ReplicatedUsing="OnRep_UpdateProjectileVelocity")
	float _DynamicSpeed;

	// A replicated variable to track the pulled Actor's position and then to update accordingly.
	UPROPERTY(Replicated, ReplicatedUsing = "OnRep_PullActorToAnchor")
	FVector _TrackedActorPosition; 

public:	
	// Sets default values for this actor's properties
	AHookshotAnchor();

	// Updates the velocity of the projectile. (Server Only)
	UFUNCTION(BlueprintCallable)
	void SetSpeed(float New);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// After the "Time Until Alive" activates the collision. Exposed to public in case the end user has a preferred method.
	UFUNCTION(BlueprintCallable)
	void ActivateCollision();

	// Begins pulling the target actor.
	UFUNCTION(BlueprintCallable)
	void StartPullActor(AActor* Target);

	// Stops pulling the target actor. May also destroy the anchro.
	UFUNCTION(BlueprintCallable)
	void StopPullActor(bool DestroySelf = true);

protected:
	virtual void BeginPlay() override;

	// Updates the Velocity of the Projectile when SetSpeed is called.
	UFUNCTION()
	void OnRep_UpdateProjectileVelocity(float Old);

	// Updates the actor position on all clients when they are being pulled.
	UFUNCTION()
	void OnRep_PullActorToAnchor(const FVector& Old);

	// Updates the collision for all contexts when needed.
	UFUNCTION()
	void OnRep_UpdateCollision(bool Old);

	// Responds to the collider hitting a surface to call the correct delegate.
	UFUNCTION()
	void ReactToColliderOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
