// Fill out your copyright notice in the Description page of Project Settings.


#include "HookshotAnchor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "Net/UnrealNetwork.h"

namespace {
	auto IsServer = [](UObject* o) -> bool { return o->GetWorld()->GetAuthGameMode() != nullptr; };
};

void AHookshotAnchor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHookshotAnchor, _DynamicSpeed);
	DOREPLIFETIME(AHookshotAnchor, _PullActor);
	DOREPLIFETIME(AHookshotAnchor, _TrackedActorPosition);
	DOREPLIFETIME(AHookshotAnchor, _IsLive);
}

// Sets default values
AHookshotAnchor::AHookshotAnchor()
{
	bReplicates = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Collider = CreateDefaultSubobject<USphereComponent>("Collision");
	Collider->SetSphereRadius(8.f);
	Collider->SetGenerateOverlapEvents(false);
	Collider->SetNotifyRigidBodyCollision(true);

	// Ignore by default to reduce risk of colliding with spawner.
	Collider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SetRootComponent(Collider);

	Arrow = CreateDefaultSubobject<UArrowComponent>("Arrow");
	Arrow->SetupAttachment(Collider);

	// Remove gravity so it flies straight, this can be overriden in BP
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("Projectile Movement");
	ProjectileMovement->ProjectileGravityScale = 0.f;

	// Travel a max of 2000 units, activate collision after 0.2 seconds.
	MaxDistance = 2000.f;
	TimeUntilLive = 0.2;
	PullSpeed = 5.f;
	CutoffDistance = 500.f;

	_PullActor = nullptr;
	_SpawnLocation = FVector();
	_TrackedActorPosition = FVector();

	_IsLive = false;
	
}

void AHookshotAnchor::BeginPlay() {
	Super::BeginPlay();
	_SpawnLocation = GetActorLocation();
	if (IsServer(this)) {
		Collider->OnComponentHit.AddDynamic(this, &AHookshotAnchor::ReactToColliderOnHit);
		if (_IsLive) {
			ActivateCollision();
		} else {
			FTimerHandle ThrowAway;
			GetWorld()->GetTimerManager().SetTimer(ThrowAway, this, &AHookshotAnchor::ActivateCollision, TimeUntilLive, false);
		}
	}
}

void AHookshotAnchor::ActivateCollision() {
	if (IsServer(this)) {
		_IsLive = !_IsLive;
		OnRep_UpdateCollision(!_IsLive);
	}
}

void AHookshotAnchor::OnRep_UpdateCollision(bool Old) {
	Collider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
}

// Called every frame
void AHookshotAnchor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	auto KillAtMaxDistance = [this]() -> bool {
		float DistanceFromSpawn = (_SpawnLocation - GetActorLocation()).Size();
		if (DistanceFromSpawn >= MaxDistance) {
			StopPullActor(true);
			return true;
		}
		return false;
	};
	auto KillAtDestination = [this]() -> bool {
		if (_PullActor) {
			float RemainingDistance = (GetActorLocation() - _PullActor->GetActorLocation()).Size();
			if (RemainingDistance <= CutoffDistance) {
				StopPullActor(true);
				return true;
			}
		}
		return false;
	};
	auto TickPullActor = [this](float DeltaTime) {
		if (_PullActor) {
			// Interp Location of pulled actor.
			FVector OldPosition = _PullActor->GetActorLocation();
			_TrackedActorPosition = FMath::VInterpTo(_PullActor->GetActorLocation(), GetActorLocation(), DeltaTime, PullSpeed);
			OnRep_PullActorToAnchor(OldPosition);
		}
	};
	if (IsServer(this) && !IsPendingKill()) {
		if (!KillAtMaxDistance()) {
			if (!KillAtDestination()) {
				TickPullActor(DeltaTime);
			}
		}
	}
}

void AHookshotAnchor::StartPullActor(AActor* Target) {
	if (IsServer(this) && !_PullActor) {
		_PullActor = Target;
	}
}

void AHookshotAnchor::StopPullActor(bool DestroySelf) {
	if (IsServer(this)) {
		_PullActor = nullptr;
		if (DestroySelf) {
			OnAnchorExpire.Broadcast(this, GetActorTransform());
			Destroy();
		}
	}
}

void AHookshotAnchor::SetSpeed(float New) {
	if (IsServer(this)) {
		float OldSpeed = _DynamicSpeed;
		_DynamicSpeed = New;
		OnRep_UpdateProjectileVelocity(OldSpeed);
	}
}

void AHookshotAnchor::OnRep_UpdateProjectileVelocity(float Old) {
	ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * _DynamicSpeed;
}

void AHookshotAnchor::OnRep_PullActorToAnchor(const FVector& Old) {
	if (_PullActor) {
		// Update Actor Location and alert if the pull was blocked to resolve properly.
		FHitResult Hit;
		_PullActor->SetActorLocation(_TrackedActorPosition, true, &Hit);
		if (Hit.bBlockingHit) {
			OnPullBlocked.Broadcast(this, Hit);
		}
	}
}

void AHookshotAnchor::ReactToColliderOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {
	if (IsServer(this) && !_PullActor) {
		OnHookshotHit.Broadcast(this, Hit);
	}
}