// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

UCLASS()
class CE301CAPSTONEPROJECT_API AProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectileBase();

protected:
	virtual void BeginPlay() override; //Called when projectile is spawned

private:
	UPROPERTY(VisibleAnywhere)
		class UProjectileMovementComponent* ProjectileMovement;
	UPROPERTY(EditDefaultsOnly)
		class USphereComponent* ProjectileCollider;

	//Called when projectile hits a component
	UFUNCTION()
		void OnCollision(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);
	UFUNCTION()
		void ActorHitDetected(UPrimitiveComponent* hitComponent, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hitResult);

	UPROPERTY(VisibleAnywhere)
		class UParticleSystemComponent* projectileTrailParticleEffect; //Projectile's particle effects
	UPROPERTY(EditAnywhere)
		class UNiagaraSystem* hitParticleEffect;

	UPROPERTY(EditDefaultsOnly)
		class USoundBase* FireLoopSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* FireExplosionSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* PerfectBlockSFX;
	UPROPERTY(EditAnywhere)
		TSubclassOf<UCameraShakeBase> projectileShakeClass;

	void Invert(AActor* deflectActor);
	void checkUpcomingCollision();

	class UAudioComponent* loopingFire;

	FTimerDelegate checkUpcomingCollisionTimer;
	FTimerHandle checkUpcomingCollisionHandle;

	AActor* lastTouchActor;

	UPROPERTY(EditAnywhere)
		float damage = 25.f;

	UPROPERTY(EditAnywhere)
		float maxSpeed = 4000;
};
