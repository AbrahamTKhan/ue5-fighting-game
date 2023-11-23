// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "EnemyBoss.generated.h"

/**
 * 
 */
UCLASS()
class CE301CAPSTONEPROJECT_API AEnemyBoss : public AEnemyBase
{
	GENERATED_BODY()
public:

	AEnemyBoss();

protected:

	virtual void BeginPlay() override;

public:

	virtual void Tick(float DeltaTime) override;

protected:
	void AIStateMachine() override;
	void S_Passive() override;
	void S_Attack() override;
	void S_CombatApproach() override;
	void S_CombatRanged() override;
	void UpdateAI() override;
	void FinishHitReaction() override;

	void AIStartApproaching();
	void RandomCombatType();

	bool approachOverride = true;
};
