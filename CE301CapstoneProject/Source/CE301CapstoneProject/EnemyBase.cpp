// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyBase.h"
#include "PlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "CE301CapstoneProjectGameModeBase.h"
#include "ProjectileBase.h"
#include "PlayerCharacter.h"
#include "EnemySpawner.h"
#include "CapstoneGameInstance.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Spawn Point"));
	ProjectileSpawnPoint->SetupAttachment(GetMesh());

	bestAvoidLocation = GetActorLocation();
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	combatType = startingCombatType;
	if (combatType != 2)
	{
		MeleeWeapon->SetHiddenInGame(true);
	}

	AIController = Cast<AAIController>(Controller); //Casts enemy's controller to AIController for extra functionality
	CurrentAIState = State::IDLE;
	
	navigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());

	if (keepBlocking) //Tutorial states
	{
		Block();
	}
	else if (!idle)
	{
		updateAITimer = FTimerDelegate::CreateUObject(this, &AEnemyBase::GetNewTarget, true);
		GetWorldTimerManager().SetTimer(updateAIHandle, updateAITimer, 0.25f, false);
	}

	if (gameInstance) //Changes AI tendencies depending on the difficulty
	{
		int difficulty = gameInstance->GetDifficulty();
		if (difficulty == 0)
		{
			AdjustDifficultyVariables(0.25f);
			BaseMovementSpeed = 400.0f;
			FocusedMovementSpeed = 225.0f;
			RunningSpeed = 550.f;
		}
		else if (difficulty == 1)
		{
			AdjustDifficultyVariables(0.5f);
			BaseMovementSpeed = 400.0f;
			FocusedMovementSpeed = 225.0f;
			RunningSpeed = 550.f;
		}
		else if (difficulty == 3)
		{
			lightAttackChance = 65;
			heavyAttackChance = 40;
			specialAttackChance = 10;
			blockBreakerChance = 70;

			blockChance = 40;
			perfectChance = 35;
			evadeChance = 50;
			counterAttackChance = 75;
			wallChance = 40;
		}
		else if (difficulty == 4)
		{
			lightAttackChance = 65;
			heavyAttackChance = 40;
			specialAttackChance = 10;
			blockBreakerChance = 70;

			blockChance = 45;
			perfectChance = 45;
			evadeChance = 60;
			counterAttackChance = 90;
			wallChance = 50;
		}
	}
}



void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Target)
	{
		AIStateMachine();

		RotateToTarget();
	}

	if (enemyDebug)
	{
		PrintStates();
	}
}

void AEnemyBase::AIStateMachine()
{
	switch (CurrentAIState)
	{
	case State::IDLE:
		S_Passive();
		break;

	case State::COMBAT_RANGED:
		S_CombatRanged();
		break;

	case State::COMBAT_APPROACH:
		S_CombatApproach();
		break;

	case State::ATTACK:
		S_Attack();
		break;

	case State::AVOID:
		S_Avoid();
		break;

	case State::EVADE:
		S_Evade();
		break;

	case State::GROUNDED:
		S_Grounded();
		break;
	}
}

void AEnemyBase::SetAIState(State state)
{
	if (!IsFighterDead())
	{
		CurrentAIState = state;

		if (state != State::AVOID && state != State::GROUNDED)
		{
			GetWorldTimerManager().ClearTimer(updateAIHandle);
			GetWorldTimerManager().SetTimer(updateAIHandle, updateAITimer, 0.25f, true);
		}

		if (state == State::COMBAT_APPROACH)
		{
			AFighterBase::StopBlocking();
			AIController->MoveToActor(Target);
			RunCheck();
		}
	}
}

void AEnemyBase::UpdateAI() //Handles AI's action depending on their state
{
	if (CurrentAIState == State::COMBAT_RANGED)
	{
		if ((!isAttacking || (isAttacking && canCombo)) && !isReactingToHit && !isEvading && stamina > 30.f)
		{
			targetDistance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
			if (targetDistance < 300.f) //Magic actors will try to block break (kick) if an enemy gets too close
			{
				float randomBlockBreaker = FMath::Rand() % 100;
				if (randomBlockBreaker < blockBreakerChance * 2)
				{
					BlockBreaker();

					RunCheck();
					bestAvoidLocation = GetAIReachableLocation(true);
					AIController->MoveToLocation(bestAvoidLocation);
				}
			}
			else if (!AIController->IsFollowingAPath()) //Keep moving to new locations
			{
				bestAvoidLocation = GetAIReachableLocation(false);
				AIController->MoveToLocation(bestAvoidLocation);
			}

			bool shouldFire = true;

			FVector targetVelocity = Target->GetVelocity();
			FVector targetVelocityLocation = (Target->GetActorLocation() + (targetVelocity * 0.5f)) - GetActorLocation(); //Calculates where the target will be based on their speed and direction

			FHitResult hitResult;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this); //Performs a line trace to see if the target is within sight
			if (GetWorld()->LineTraceSingleByChannel(hitResult, GetActorLocation() + (GetActorForwardVector() * 2), Target->GetActorLocation(), ECollisionChannel::ECC_Visibility, Params))
			{
				if (!hitResult.GetActor()->IsA<APlayerCharacter>())
				{
					shouldFire = false;
				}
				//Performs another line trace to see if the target will be out of sight soon
				if (GetWorld()->LineTraceSingleByChannel(hitResult, GetActorLocation() + (GetActorForwardVector() * 2), (Target->GetActorLocation() + (targetVelocity * 0.5f)), ECollisionChannel::ECC_Visibility, Params))
				{
					if (!hitResult.GetActor()->IsA<APlayerCharacter>())
					{
						shouldFire = false;
					}
				}
			}
			if (shouldFire)
			{
				float randomAttack = FMath::Rand() % 100;

				if (isAttacking)
				{
					randomAttack += 5;
				}
				FVector targetDirection = Target->GetActorLocation() - GetActorLocation();

				attackAngle = FVector::DotProduct(GetActorForwardVector(), targetDirection.GetSafeNormal()); //Gets how close the AI's front facing direction is to the position of the target
				if (attackAngle > 0.95f) //Allows AI to attack if they are facing in the correct direction
				{
					if (randomAttack < heavyAttackChance * aggressionMultiplier / passivenessMultiplier)
					{
						HeavyAttack();
					}
					else if (randomAttack < lightAttackChance * aggressionMultiplier / passivenessMultiplier)
					{
						LightAttack();
					}
				}
			}
			
		}
	}

	else if (CurrentAIState == State::ATTACK)
	{
		if ((!isAttacking || (isAttacking && canCombo)) && !isReactingToHit && !isEvading)
		{
			targetDistance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
			FVector targetDirection = Target->GetActorLocation() - GetActorLocation();
			attackAngle = FVector::DotProduct(GetActorForwardVector(), targetDirection.GetSafeNormal()); //Gets how close the AI's front facing direction is to the position of the target
			if (attackAngle > 0.95f) //Allows AI to attack if they are facing in the correct direction
			{
				float randomAttack = (FMath::Rand() % 100);
				if (targetDistance < 200.f) //Randomly selects attack
				{
					if ((combatType == 1 && randomAttack < specialAttackChance * aggressionMultiplier / passivenessMultiplier) || (combatType == 2 && randomAttack < (specialAttackChance - 2.5f) * aggressionMultiplier / passivenessMultiplier))
					{
						SpecialAttack();
					}
					else if (randomAttack < heavyAttackChance * aggressionMultiplier / passivenessMultiplier)
					{
						HeavyAttack();
					}
					else if (randomAttack < lightAttackChance * aggressionMultiplier / passivenessMultiplier)
					{
						LightAttack();
					}
					else if (randomAttack < blockBreakerChance * aggressionMultiplier / passivenessMultiplier)
					{
						BlockBreaker();
					}
				}
				else if(targetDistance < 275.f)
				{
					if (randomAttack < (specialAttackChance + 2.5f) * aggressionMultiplier / passivenessMultiplier)
					{
						SpecialAttack();
					}
				}
			}
		}
	}
	else if (CurrentAIState == State::GROUNDED)
	{
		SetAIState(State::IDLE);
	}
}

void AEnemyBase::S_Passive() //Does nothing unless target is in range
{
	if (FVector::Distance(Target->GetActorLocation(), GetActorLocation()) < detectionRange)
	{
		if (combatType == 3) //Magic enemies enter ranged combat
		{
			SetAIState(State::COMBAT_RANGED);
		}
		else //Melee enemies approach the target
		{
			SetAIState(State::COMBAT_APPROACH);
		}
	}
}



void AEnemyBase::S_CombatRanged()
{
}

void AEnemyBase::S_Grounded() //Automatically gets up when grounded
{
	if (isFrontGrounded)
	{
		PlayAnimMontage(RecoveryAnimations[0]);
		isFrontGrounded = false;
		GetWorldTimerManager().SetTimer(updateAIHandle, updateAITimer, 1.f, true);
	}
	else if (isBackGrounded)
	{
		PlayAnimMontage(RecoveryAnimations[1]);
		isBackGrounded = false;
		GetWorldTimerManager().SetTimer(updateAIHandle, updateAITimer, 1.f, true);
	}

}

void AEnemyBase::S_Evade()
{
}

void AEnemyBase::S_CombatApproach()
{
	targetDistance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
	if (targetDistance <= 280.f) //Starts to attack when within range
	{
		EndRun();
		SetAIState(State::ATTACK);
	}
}

void AEnemyBase::S_Attack()
{
	targetDistance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
	if (targetDistance > 450.f) //Reverts to approaching when out of range
	{
		RunCheck();
		SetAIState(State::COMBAT_APPROACH);
	}
}

void AEnemyBase::S_Avoid()
{
	if (stamina > maxStamina * 0.4) //Changes back to idle once stamina is high enough
	{
		SetAIState(State::IDLE);
		startAvoid = false;
	}
	else if(startAvoid && !isAttacking) //When the AI is first ready to avoid, generate new avoid location
	{
		EndRun();
		bestAvoidLocation = GetAIReachableLocation(true);
		AIController->MoveToLocation(bestAvoidLocation);
		turnTowardsTarget = true;
		lockedTurnSpeed = 10.f;
		startAvoid = false;
	}
}

FVector AEnemyBase::GetAIReachableLocation(bool avoid) //avoid parameter changes the acceptable direction threshold
{
	int count = 0;
	bestAvoidLocation = GetActorLocation();
	if (Target)
	{
		while (count <= 10)
		{
			FNavLocation reachablePoint;
			
			navigationSystem->GetRandomReachablePointInRadius(GetActorLocation(), 3000.f, reachablePoint); //Generates a reachable point in the nav mesh

			FVector avoidTargetDirection = Target->GetActorLocation() - GetActorLocation();
			FVector avoidPointDirection = reachablePoint.Location - GetActorLocation();
			avoidTargetDirection.Normalize(); avoidPointDirection.Normalize();
			float avoidAngle = FVector::DotProduct(avoidTargetDirection, avoidPointDirection); //Gets the angle between the direction to the target and the direction to the reachable point

			targetDistance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
			if (((avoid && avoidAngle <= 0.2f) || (!avoid && avoidAngle <= 0.6f) && FVector::Dist(GetActorLocation(), reachablePoint.Location) > FVector::Dist(GetActorLocation(), bestAvoidLocation)))
			{
				bestAvoidLocation = reachablePoint.Location; //Prioritises locations that move the AI away from the target especially when avoid = true
			}

			count++;
		}
	}
	/*DrawDebugSphere(GetWorld(), bestAvoidLocation, 20.f, 10, FColor::Yellow, true, -1.f, 0U, 10.f);*/
	return bestAvoidLocation;
}

void AEnemyBase::Attack()
{
	Super::Attack();

	//Check if stamina has been depleted enough to require the avoid state
	if (combatType == 3 && stamina <= maxStamina / 5 && Target && FVector::Dist(GetActorLocation(), Target->GetActorLocation()) < requiredDistanceToAvoid)
	{
		GetWorldTimerManager().ClearTimer(updateAIHandle);

		if (isBlocking)
		{
			AFighterBase::StopBlocking();
		}
		AIController->StopMovement();
		startAvoid = true;
		SetAIState(State::AVOID);
	}
	else if (combatType != 3)
	{
		AIController->StopMovement();
		lockedTurnSpeed = 1.f;
	}
	CalculateAggression();
	
	if (!IsFighterDead())
	{
		updateHUD();
	}
}

void AEnemyBase::Evade()
{
	
	PlayAnimMontage(EvadeAnimations[evadeIndex]);
	
	Super::Evade();
}

void AEnemyBase::FinishEvade()
{
	Super::FinishEvade();

	if (combatType == 3)
	{
		SetAIState(State::COMBAT_RANGED);
	}
	else
	{
		SetAIState(State::COMBAT_APPROACH);
	}
}

void AEnemyBase::SetAttackAppliesDamage(bool isDamaging)
{
	Super::SetAttackAppliesDamage(isDamaging);

	//Check if stamina has been depleted enough to require the avoid state
	if (isDamaging && stamina <= maxStamina / 5.f && Target && FVector::Dist(GetActorLocation(), Target->GetActorLocation()) < requiredDistanceToAvoid)
	{
		GetWorldTimerManager().ClearTimer(updateAIHandle);

		if (isBlocking)
		{
			AFighterBase::StopBlocking();
		}
		startAvoid = true;
		SetAIState(State::AVOID);
	}

	if (!IsFighterDead())
	{
		updateHUD();
	}
}


void AEnemyBase::StopAttacking()
{
	Super::StopAttacking();
	
	turnTowardsTarget = true;
	lockedTurnSpeed = 15.f;
	RunCheck();
	if (combatType != 3 && Target && !AIController->IsFollowingAPath())
	{
		AIController->MoveToActor(Target);
	}
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	turnTowardsTarget = true;
	lockedTurnSpeed = 15.f;
	if (!IsFighterDead())
	{
		updateHUD();

		CalculateAggression();
		CalculatePassiveness();

		SetDamageTarget(DamageCauser); //Changes the target to the damage actor
		if (!isEvading)
		{
			AIController->StopMovement();
		}
	}
	else
	{
		AEnemySpawner* spawner = Cast<AEnemySpawner>(GetOwner());
		if (gameInstance && gameMode == 5 || gameMode == 6 && spawner)
		{
			spawner->SpawnEnemy(-1); //Spawn new enemy if the game mode requires so
		}

		GetWorldTimerManager().ClearTimer(updateAIHandle);
		DetachFromControllerPendingDestroy();
	}

	return damageRecieved;
}

void AEnemyBase::GetNewTarget(bool atSpawn) //Gets a target either at spawn or when the current target dies
{
	float nearestEnemy = detectionRange;
	for (APlayerCharacter* Player : TActorRange<APlayerCharacter>(GetWorld()))
	{
		if (Player && !Player->IsFighterDead() && FVector::Dist(GetActorLocation(), Player->GetActorLocation()) < nearestEnemy) 
		{
			Target = Player;
			nearestEnemy = FVector::Dist(GetActorLocation(), Player->GetActorLocation()); //Gets the nearest player
		}
	}
	if (Target)
	{
		SetAIState(State::IDLE);
		isTargetFocused = true;
		idle = false;
	}

	if (atSpawn)
	{
		updateAITimer = FTimerDelegate::CreateUObject(this, &AEnemyBase::UpdateAI);
	}
}

void AEnemyBase::RunCheck() //Determines if the AI has enough stamina to run at the target
{
	if (Target)
	{
		if (stamina > maxStamina * 0.4 && FVector::Dist(GetActorLocation(), Target->GetActorLocation()) > 300)
		{
			StartRun();
		}
		else
		{
			EndRun();
		}
	}
}

void AEnemyBase::ReactToAttack(AActor* damageActor, int attackerCombatType, int attackerAttackType) //Notifies AI fighters of an upcoming attack to allow them to generate a 'response'
{
	if ((!isReactingToHit || canBlock) && !isEvading && !isFrontGrounded && !isBackGrounded && !isFallingOver && !isAttacking && !idle && !keepBlocking && gameMode != 5)
	{
		if (damageActor != Target)
		{
			SetDamageTarget(damageActor);
		}

		FVector damageDirection = damageActor->GetActorLocation() - GetActorLocation();
		float avoidAngle = FMath::Abs(FVector::DotProduct(GetActorForwardVector(), damageDirection.GetSafeNormal()));
		staminaDrained = 0;
		if (avoidAngle > 0.25f) //Only react if the attack is not from behind
		{
			if (damageActor->IsA<AProjectileBase>()) //Projectile reaction
			{
				float randomBlock = FMath::Rand() % 100;
				float randomPerfectBlock = FMath::Rand() % 100;
				float randomEvade = FMath::Rand() % 100;
				float randomWall = FMath::Rand() % 100;

				if (randomBlock < blockChance * passivenessMultiplier / aggressionMultiplier)
				{
					Block();
				}
				else if (randomEvade < evadeChance * passivenessMultiplier / aggressionMultiplier)
				{
					isEvading = true;
					isMovingForward = true;

					int randNum = FMath::RandRange(0, 1);
					int evadeYaw = 90;
					if (randNum == 0) //Random evade direction
					{
						evadeIndex = 5;
					}
					else
					{
						evadeIndex = 6;
						evadeYaw = -90;
					}
					staminaDrained = FMath::Min(stamina, runEvadeStaminaCost);
					FRotator rotateDirection(0, evadeYaw, 0);
					weightedInputDirection = rotateDirection.RotateVector(damageDirection); //Gets 90 degree angle from the direction of the attack
					Evade();
				}
				else if (randomPerfectBlock < perfectChance * passivenessMultiplier / aggressionMultiplier)
				{
					PerfectBlock();
				}
				else if (combatType == 3 && stamina / maxStamina > 0.3f && randomWall < wallChance * passivenessMultiplier / aggressionMultiplier)
				{
					SpecialAttack();
				}
			}
			else if (damageActor->IsA<AEnemyBase>()) //Enemy reaction, doesn't perfect block here
			{
				float randomBlock = FMath::Rand() % 100;
				float randomEvade = FMath::Rand() % 100;

				if (randomBlock < blockChance / 1.4f)
				{
					Block();
				}
				else if (randomEvade < evadeChance / 1.4f)
				{
					isEvading = true;
					isMovingForward = true;

					int randNum = FMath::RandRange(0, 1);
					int evadeYaw = 90;
					if (randNum == 0) //Random evade direction
					{
						evadeIndex = 5;
					}
					else
					{
						evadeIndex = 6;
						evadeYaw = -90;
					}

					staminaDrained = FMath::Min(stamina, runEvadeStaminaCost);
					FRotator rotateDirection(0, evadeYaw, 0);
					weightedInputDirection = rotateDirection.RotateVector(damageDirection); //Gets 90 degree angle from the direction of the attack
					Evade();
				}
			}
			else //Player reaction
			{
				float attackerActionMultiplier = 1;
				float combatTypeMultiplier = 1;

				float randomBlock = FMath::Rand() % 100;
				float randomPerfectBlock = FMath::Rand() % 100;
				float randomEvade = FMath::Rand() % 100;

				if (attackerCombatType == 2) //More likely to react to weapon attacks
				{
					if (attackerAttackType == 3) //Even more likely to react to the slow special attack
					{
						attackerActionMultiplier = 2.f;
					}
					else
					{
						attackerActionMultiplier = 1.2f;
					}
				}
				if (combatType == 2) //More likely to react based on the AI's combat type
				{
					combatTypeMultiplier = 1.3f;
				}
				else if (combatType == 3)
				{
					combatTypeMultiplier = 2.f;
				}

				if (randomEvade < evadeChance * attackerActionMultiplier * combatTypeMultiplier * (passivenessMultiplier / 2) / aggressionMultiplier)
				{
					isEvading = true;


					float randomEvadeType = FMath::Rand() % 100;
					int evadeYaw = 90;
					if (randomEvadeType > 90) //Horizontal evade right
					{
						evadeIndex = 5;
						isMovingForward = true;
						staminaDrained = FMath::Min(stamina, runEvadeStaminaCost);
						FRotator rotateDirection(0, evadeYaw, 0);
						weightedInputDirection = rotateDirection.RotateVector(damageDirection);
					}
					else if (randomEvadeType > 80) //Horizontal evade left
					{
						evadeIndex = 6;
						evadeYaw = -90;
						isMovingForward = true;
						staminaDrained = FMath::Min(stamina, runEvadeStaminaCost);
						FRotator rotateDirection(0, evadeYaw, 0);
						weightedInputDirection = rotateDirection.RotateVector(damageDirection);
					}
					else if (randomEvadeType > 70) //Backwards evade
					{
						evadeIndex = 2;
						evadeYaw = 180;
						isMovingBackwards = true;
						staminaDrained = FMath::Min(stamina, runEvadeStaminaCost);
						weightedInputDirection = -GetActorForwardVector();
					}
					else //Stationary evade
					{
						if (evadeIndex == 3)
						{
							evadeIndex = 7;
						}
						else
						{
							evadeIndex = 3;
						}
						evadeYaw = 0;
						staminaDrained = FMath::Min(stamina, standingEvadeStaminaCost);
						weightedInputDirection = FVector(0,0,0);
					}
					Evade();
				}
				else if (randomBlock < blockChance * attackerActionMultiplier * combatTypeMultiplier * (passivenessMultiplier / 2) / aggressionMultiplier)
				{
					Block();
				}
				
				else if (randomPerfectBlock < perfectChance * attackerActionMultiplier * combatTypeMultiplier * (passivenessMultiplier / 2) / aggressionMultiplier)
				{
					PerfectBlock();
				}
			}
		}

		stamina -= staminaDrained;
	}
	
}

void AEnemyBase::SetTarget(AActor* fighterTarget)
{
	Super::SetTarget(fighterTarget);
	if (Target == nullptr)
	{
		GetWorldTimerManager().ClearTimer(updateAIHandle);
	}
}

void AEnemyBase::SetIsFallingOver(bool result)
{
	Super::SetIsFallingOver(result);

	GetWorldTimerManager().ClearTimer(updateAIHandle);
	AIController->StopMovement();
	SetAIState(State::GROUNDED);
}

void AEnemyBase::CanRecover(bool can) //Automatically recover if the AI has enough stamina
{
	if (stamina >= 40.f)
	{
		Super::Evade();

		if (AIController->IsFollowingAPath())
		{
			AIController->StopMovement();
		}

		canRecover = false;
		isEvading = true;
		isFallingOver = false;
		isReactingToHit = false;

		staminaDrained = FMath::Min(stamina, recoverStaminaCost);
		stamina -= staminaDrained;
		
		if (isMovingForward)
		{
			weightedInputDirection = GetActorForwardVector();
			PlayAnimMontage(RecoveryAnimations[2]);
		}
		else if (isMovingBackwards)
		{
			weightedInputDirection = -GetActorForwardVector();
			PlayAnimMontage(RecoveryAnimations[3]);
		}
	}
}

void AEnemyBase::CanCounter()
{
	Super::CanCounter();

	if (canCounter) //Random chance at countering
	{
		float randCounter = FMath::Rand() % 100;
		if (randCounter > counterAttackChance * aggressionMultiplier / passivenessMultiplier)
		{
			if (stamina / maxStamina < 0.6f)
			{
				attackType = 1;
			}
			else
			{
				attackType = 2;
			}
			Attack();
		}
	}
}

void AEnemyBase::RotateToTarget() //Makes actor smoothly turn towards the target
{
	if (Target)
	{
		FVector actorDirection = Target->GetActorLocation() - GetActorLocation(); //Subtracting two positional vectors gets direction vector
		actorDirection.Z = 0.f; //Shouldn't affect Z axis
		FRotator actorRotation = FRotationMatrix::MakeFromX(actorDirection).Rotator(); //Creates the desired actor rotation relative to target
		FRotator smoothTurn = FMath::Lerp(GetActorRotation(), actorRotation, lockedTurnSpeed * GetWorld()->DeltaTimeSeconds); //Lerp smoothly rotates actor
		/*lastRotationRate = smoothTurn.Yaw - GetActorRotation().Yaw; */
		SetActorRotation(smoothTurn); //Updates rotation
	}
}

void AEnemyBase::FinishHitReaction()
{
	Super::FinishHitReaction();

	RunCheck();

	if (combatType != 3 && Target && !AIController->IsFollowingAPath() && !idle && !keepBlocking && gameMode != 5)
	{
		AIController->MoveToActor(Target);
	}
	else if (keepBlocking)
	{
		PlayAnimMontage(BlockAnim);
	}
}

void AEnemyBase::StopBlocking()
{
	Super::StopBlocking();

	RunCheck();

	if (combatType != 3 && Target && !AIController->IsFollowingAPath())
	{
		AIController->MoveToActor(Target);
	}

}

void AEnemyBase::CanBlock(bool can) //Automatically reblock
{
	if (gameMode != 5 && !idle && !keepBlocking)
	{
		canBlock = true;
		Block();

		if (Target && combatType != 3)
		{
			AIController->MoveToActor(Target);
		}
	}
}

void AEnemyBase::SpawnMagicActor() //Rotate the projectile spawn point to face the target as a projectile is spawned
{
	FVector targetVelocity = Target->GetVelocity();
	FVector targetVelocityLocation = (Target->GetActorLocation() + (targetVelocity * 0.5f)) - GetActorLocation();
	FRotator Rot = FRotationMatrix::MakeFromX(targetVelocityLocation).Rotator();
	ProjectileSpawnPoint->SetWorldRotation(Rot);
	Super::SpawnMagicActor();
}

void AEnemyBase::AdjustDifficultyVariables(float multiplier)
{
	kickLightDamage *= multiplier;
	kickMediumDamage *= multiplier;
	kickSpecialDamage *= multiplier;
	weaponLightDamage *= multiplier;
	weaponMediumDamage *= multiplier;
	weaponSpecialDamage *= multiplier;
	counterDamage *= multiplier;

	lightAttackChance *= multiplier;
	heavyAttackChance *= multiplier;
	specialAttackChance *= multiplier;
	blockBreakerChance *= multiplier;

	blockChance *= multiplier;
	perfectChance *= multiplier;
	evadeChance *= multiplier;
	counterAttackChance *= multiplier;
	wallChance *= multiplier;
}


void AEnemyBase::SetDamageTarget(AActor* damageActor) //Change target if damaged by someone other than the current target
{
	if (damageActor && !idle && !keepBlocking && gameMode != 5)
	{
		if (AProjectileBase* projectile = Cast<AProjectileBase>(damageActor))
		{
			APlayerCharacter* player = Cast<APlayerCharacter>(projectile->GetOwner());
			if (player)
			{
				Target = player;
			}
		}
		else if (APlayerCharacter* player = Cast<APlayerCharacter>(damageActor))
		{
			Target = player;
		}
	}
}

void AEnemyBase::CalculateAggression() //Adjusts aggression based on the target's health and stamina
{
	if (AFighterBase* fighterTarget = Cast<AFighterBase>(Target))
	{
		float targetHealth = fighterTarget->GetHealthAmount();
		float targetStamina = fighterTarget->GetStaminaAmount();
		float healthAggression = 1 + (1 - targetHealth);
		float staminaAggression = 1 + (1 - targetStamina);
		aggressionMultiplier = (healthAggression + staminaAggression) / 2;
	}
}

void AEnemyBase::CalculatePassiveness() //Adjusts passiveness based on the AI's health and stamina
{
	if (AFighterBase* fighterTarget = Cast<AFighterBase>(Target))
	{
		float healthPercent = health / maxHealth;
		float staminaPercent = stamina / maxStamina;
		float healthPassiveness = 1 + (1 - healthPercent);
		float staminaPassiveness = 1 + (1 - staminaPercent);
		passivenessMultiplier =  (healthPassiveness + staminaPassiveness) / 2;
	}
}

void AEnemyBase::PrintStates()
{
	Super::PrintStates();
}

void AEnemyBase::SetDebug(bool cond)
{
	enemyDebug = cond;
}
