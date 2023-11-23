#include "EnemyBoss.h"
#include "EnemyBase.h"
#include "PlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/World.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AEnemyBoss::AEnemyBoss()
{
	PrimaryActorTick.bCanEverTick = true;

	combatType = 3;
}

void AEnemyBoss::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyBoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemyBoss::UpdateAI()
{
	if (CurrentAIState == State::COMBAT_RANGED)
	{
		float randomApproach = FMath::Rand() % 100;
		if (randomApproach > 95 && stamina / maxStamina > 0.4f && FVector::Dist(Target->GetActorLocation(), GetActorLocation()) < 2000.f)
		{
			AIStartApproaching(); //Small chance to randomly start approaching the target even if they're not close
			return;
		}
		else if (!AIController->IsFollowingAPath()) //Keep moving to new locations
		{
			bestAvoidLocation = GetAIReachableLocation(false);
			AIController->MoveToLocation(bestAvoidLocation);
		}

		else if ((!isAttacking || (isAttacking && canCombo)) && !isReactingToHit && stamina > 30.f)
		{
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
					if (randomAttack < heavyAttackChance * aggressionMultiplier / (passivenessMultiplier * 4))
					{
						HeavyAttack();
					}
					else if (randomAttack < lightAttackChance * aggressionMultiplier / (passivenessMultiplier * 4))
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
				else if (targetDistance < 275.f)
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

void AEnemyBoss::AIStateMachine()
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

void AEnemyBoss::S_Passive() //Does nothing unless target is in range
{
	if (FVector::Distance(Target->GetActorLocation(), GetActorLocation()) < 2500.f)
	{
		MagicCombatToggle();

		SetAIState(State::COMBAT_RANGED); //Automatically go in to ranged comba
	}
}

void AEnemyBoss::S_CombatRanged()
{
	targetDistance = FVector::Distance(Target->GetActorLocation(), GetActorLocation());
	if (targetDistance < 600.f && !approachOverride)
	{
		AIStartApproaching();
	}
	else if (targetDistance > 800.f) //Require AI to move a certain distance before considering re-approaching
	{
		approachOverride = false;
	}
}

void AEnemyBoss::S_CombatApproach()
{
	targetDistance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
	if (targetDistance <= 300.f) //Melee attack threshold
	{
		EndRun();
		SetAIState(State::ATTACK);
	}
	else if (targetDistance >= 900.f && approachOverride == false) 
	{
		MagicCombatToggle();

		AIController->StopMovement();
		approachOverride = true;
		SetAIState(State::COMBAT_RANGED);
	}

	if (targetDistance < 700.f && approachOverride == true) //Require AI to move a certain distance before considering going back to ranged attacks
	{
		approachOverride = false;
	}
}

void AEnemyBoss::S_Attack()
{
	targetDistance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
	if (targetDistance > 450.f)
	{
		RunCheck();
		SetAIState(State::COMBAT_APPROACH);
	}
}

void AEnemyBoss::AIStartApproaching() //Random melee type check before approach
{
	isTargetFocused = true;
	int randType = FMath::RandRange(1, 2);
	if (randType == 1)
	{
		KickboxingCombatToggle();
	}
	else
	{
		WeaponCombatToggle();
	}
	approachOverride = true;

	SetAIState(State::COMBAT_APPROACH);;
}


void AEnemyBoss::FinishHitReaction()
{
	Super::FinishHitReaction();
	if (!IsFighterDead())
	{
		RandomCombatType();
	}
}

void AEnemyBoss::RandomCombatType() 
{
	if (CurrentAIState != State::COMBAT_RANGED)
	{
		if (combatType == 3) //Always change to a melee combat type if hit whilst using the ranged
		{
			int randType = FMath::RandRange(1, 2);
			targetDistance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
			if (targetDistance < 500 && randType == 1)
			{
				KickboxingCombatToggle();
			}
			else
			{
				WeaponCombatToggle();
			}
		}
		else //Small chance of changing to a different combat type
		{
			bool shouldChange = (FMath::Rand() % 100) > 90;
			int randType = FMath::RandRange(1, 3);
			if (shouldChange && combatType != randType)
			{
				if (randType == 1)
				{
					KickboxingCombatToggle();
				}
				else if (randType == 2)
				{
					WeaponCombatToggle();
				}
				else
				{
					MagicCombatToggle();

					RunCheck();
					bestAvoidLocation = GetAIReachableLocation(true);
					AIController->MoveToLocation(bestAvoidLocation);
					approachOverride = true;
					AFighterBase::StopBlocking();
					SetAIState(State::COMBAT_RANGED);
				}
			}
		}
	}
}

