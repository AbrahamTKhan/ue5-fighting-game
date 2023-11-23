// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FighterBase.h"
#include "EnemyBase.generated.h"


UENUM(BlueprintType)
enum class State : uint8 //Enum for custom state data type
{
	IDLE, //Target is out of range
	COMBAT_RANGED,
	COMBAT_APPROACH, //Target is within approachable range
	ATTACK, //Target is within melee distance
	AVOID,
	EVADE,
	GROUNDED,
};

UCLASS()
class CE301CAPSTONEPROJECT_API AEnemyBase : public AFighterBase
{
	GENERATED_BODY()

public:

	AEnemyBase();

protected:

	virtual void BeginPlay() override;

public:

	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	
	void ReactToAttack(AActor* damageActor, int attackerCombatType, int attackerAttackType);

	void SetDebug(bool cond);

	bool enemyDebug = false;

	UFUNCTION(BlueprintCallable)
		virtual void GetNewTarget(bool atSpawn);

protected:
	UFUNCTION(BlueprintImplementableEvent)
		void updateHUD();

	virtual void AIStateMachine(); //Ticks the current state and updates whenever a state changes
	void SetAIState(State state); //Changes state
	virtual void UpdateAI();

	virtual void S_Passive(); //----------------- Functions called on tick depending on what state the AI is in
	virtual void S_Attack();
	void S_Avoid();
	virtual void S_CombatApproach();
	virtual void S_CombatRanged();
	void S_Evade();
	void S_Grounded();

	void SetTarget(AActor* fighterTarget) override;

	void Attack() override; 
	void SetAttackAppliesDamage(bool isDamaging) override;
	void StopAttacking() override;
	void SpawnMagicActor() override;

	void Evade() override;
	void FinishEvade() override;
	virtual void FinishHitReaction() override;
	void StopBlocking() override;
	void CanBlock(bool result) override;

	void SetIsFallingOver(bool result) override;
	void CanRecover(bool can)override;
	void CanCounter() override;

	void AdjustDifficultyVariables(float multiplier);

	FVector GetAIReachableLocation(bool avoid);

	void SetDamageTarget(AActor* damageActor);

	void CalculateAggression();
	void CalculatePassiveness();

	void RotateToTarget(); //Rotates actor towards target when locked on
	void RunCheck();

	void PrintStates() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
		int startingCombatType;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State Machine") //----------------- Exposed to blueprints
		State CurrentAIState;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
		float detectionRange = 10000;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Combat")
		bool idle = false;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Combat")
		bool keepBlocking = false;

	class AAIController* AIController;
	class UNavigationSystemV1* navigationSystem;

	FTimerDelegate updateAITimer;
	FTimerHandle updateAIHandle;

	FVector bestAvoidLocation;

	float targetDistance = 0;
	float attackAngle = 0;

	float lightAttackChance = 65;
	float heavyAttackChance = 30;
	float specialAttackChance = 10;
	float blockBreakerChance = 40;

	float blockChance = 35;
	float perfectChance = 20;
	float evadeChance = 35;
	float counterAttackChance = 50;
	float wallChance = 30;

	float aggressionMultiplier = 1;
	float passivenessMultiplier = 1;

	float requiredDistanceToAvoid = 500;

	float lockedTurnSpeed = 15.f;

	bool startAvoid = false;

	bool turnTowardsTarget = true;
};
