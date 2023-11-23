// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FighterBase.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class CE301CAPSTONEPROJECT_API APlayerCharacter : public AFighterBase
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera) //Adds components to player character
		class USpringArmComponent* PlayerSpringArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		class UCameraComponent* PlayerCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera) //Sets movement speed variables
		float CameraHorizontalSensitivity = 45.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float CameraVerticalSensitivity = 45.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat") //Sphere for detecting overlapping actors
		class USphereComponent* ActorCollisionSphere;

	UFUNCTION(BlueprintImplementableEvent)
		void RemoveHUD();
	UFUNCTION(BlueprintImplementableEvent)
		void CreateDynamicMaterialInstances();

private:
	UPROPERTY(EditAnywhere, Category = Camera)
		TSubclassOf<class UMatineeCameraShake> CameraShakeOnHit; //Camera shakes for when the player hits an actor

	UFUNCTION()
		void TriggerActorOverlapInRadius(UPrimitiveComponent* OverlappedComp, //Delegate function for when an actor enters the collision sphere
			AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex, bool bFromSweep,
			const FHitResult& SweepResult);

	UFUNCTION()
		void EndActorOverlapInRadius(class UPrimitiveComponent* //Delegate function for when an actor leaves the collision sphere
			OverlappedComp, AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex);

	void CycleTarget(bool Clockwise = true); //Focuses target in different direction depending on input 
	UFUNCTION()
		void CycleTargetRight();
	UFUNCTION()
		void CycleTargetLeft();

	void MoveForward(float value);
	void MoveHorizontal(float value);
	void RotateCameraHorizontal(float value);
	void RotateCameraVertical(float value);

	void Attack() override;
	void EvaluateAttackWindow() override;
	void StopAttacking() override;

	void SetTarget(AActor* fighterTarget) override;
	AActor* UpdateNearEnemies();
	void RefreshLockedTarget();
	void ToggleEnemyLock();
	void DetectEnemiesAtSpawn();

	void WeaponCombatToggle() override;
	void KickboxingCombatToggle() override;
	void MagicCombatToggle() override;

	void FinishHitReaction() override;
	void Evade() override;

	void PauseInput();
	void ActivateTutorialBox();

	void TogglePlayerDebug();
	void ToggleEnemyDebug();
	void DisplayDebugInfo();

	class USkeletalMesh* akaiMesh;
	class UMaterial* akaiMaterial;
	class USkeletalMesh* ganfaulMesh;
	class UMaterial* ganfaulMaterial;

	FTimerDelegate enemyDetectionTimer;
	FTimerHandle enemyDetectionTimerHandle;

	TSet<class AActor*> ActorsInRange; //Set of all actors within focus range
	TSet<class AActor*> EnemiesInRange; //Specifically all enemies within focus range

	FVector inputWeight;
	FVector weightedForwardInput;
	FVector weightedHorizontalInput;

	TSubclassOf<class ATutorialBox> tBoxClass;
	
	int playerIndex = 1;

	float enemyLockRange = 5000.f;

	bool playerDebug = false;
};
