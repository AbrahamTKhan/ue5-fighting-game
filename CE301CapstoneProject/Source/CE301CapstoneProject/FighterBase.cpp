// Fill out your copyright notice in the Description page of Project Settings.

#include "FighterBase.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "CE301CapstoneProjectGameModeBase.h"
#include "ProjectileBase.h"
#include "EnemyBase.h"
#include "WallBase.h"
#include "CapstoneGameInstance.h"
#include "PlayerFighterController.h"

AFighterBase::AFighterBase() //Sets default values for variables to determine actor's initial state
{
	PrimaryActorTick.bCanEverTick = true;

	RightHandHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Right Hand Hit Box")); //Creates actor components and attaches them to the related limb 
	RightHandHitBox->SetupAttachment(GetMesh(), "RightHand");
	RightHandHitBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	RightElbowHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Right Elbow Hit Box"));
	RightElbowHitBox->SetupAttachment(GetMesh(), "RightForeArm");
	RightElbowHitBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	LeftHandHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Left Hand Hit Box"));
	LeftHandHitBox->SetupAttachment(GetMesh(), "LeftHand");
	LeftHandHitBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	LeftFootHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Left Foot Hit Box"));
	LeftFootHitBox->SetupAttachment(GetMesh(), "LeftFoot");
	LeftFootHitBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	RightFootHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Right Foot Hit Box"));
	RightFootHitBox->SetupAttachment(GetMesh(), "RightFoot");
	RightFootHitBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	MeleeWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));
	MeleeWeapon->SetupAttachment(GetMesh(), "RightHandItem");
	MeleeWeapon->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	LockOnWidgetP1 = CreateDefaultSubobject<UWidgetComponent>(TEXT("Lock On Widget P1"));
	LockOnWidgetP1->SetupAttachment(GetMesh(), "Spine2");
	LockOnWidgetP1->SetVisibility(false);
	LockOnWidgetP2 = CreateDefaultSubobject<UWidgetComponent>(TEXT("Lock On Widget P2"));
	LockOnWidgetP2->SetupAttachment(GetMesh(), "Spine2");
	LockOnWidgetP2->SetVisibility(false);

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed; //Starting speed
}

void AFighterBase::BeginPlay()
{
	Super::BeginPlay();

	health = maxHealth;
	stamina = maxStamina;

	gameInstance = GetWorld()->GetGameInstance<UCapstoneGameInstance>();
	if (gameInstance)
	{
		gameMode = gameInstance->GetGameMode(); //Stores the current game mode
	}

	playerController = Cast<APlayerFighterController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)); //Creates pointer to player controller

	resetAttackIndex = FTimerDelegate::CreateUObject(this, &AFighterBase::ResetAttackIndex); //Creates delegate timer bindings
	regenerateStamina = FTimerDelegate::CreateUObject(this, &AFighterBase::RegenerateStamina);
	perfectBlockTimer = FTimerDelegate::CreateUObject(this, &AFighterBase::UpdatePerfectBlockStatus);
	canPBTimer = FTimerDelegate::CreateUObject(this, &AFighterBase::CanPerfectBlockCheck);
	PBInputTimer = FTimerDelegate::CreateUObject(this, &AFighterBase::ClearPerfectBlockInput);
}

void AFighterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	EvaluateAttackPosition(); //Moves fighter towards their target if attacking
	EvaluateAttackWindow(); //Evaluates damage frames during an attack

	if (canRegenStamina)
	{
		stamina += staminaRegenRate * DeltaTime;
		if (stamina > maxStamina)
		{
			stamina = maxStamina;
			canRegenStamina = false;
		}
	}

	if ((isReactingToHit && (isMovingBackwards || isMovingForward))) //Automatically moves a fighter if they're in a specific state (taking damage/evading)
	{
		AddMovementInput(weightedInputDirection, 100.f * GetWorld()->GetDeltaSeconds());
	}
	else if (isEvading && isMovingBackwards)
	{
		AddMovementInput(weightedInputDirection, 150 * GetWorld()->GetDeltaSeconds()); //Moves at different speeds when evading vs taking damage
	}
	else if (isEvading && isMovingForward)
	{
		AddMovementInput(weightedInputDirection, 600.f * GetWorld()->DeltaTimeSeconds);
	}
}

void AFighterBase::Jump()
{
	if (!isEvading && !isReactingToHit && !isFrontGrounded && !isBackGrounded)
	{
		invincibilityHealth = 0.f;
		FinishEvade();
		if (!(combatType == 3 && isAttacking))
		{
			StopAnimMontage();
			if (isAttacking)
			{
				isAttacking = false;
			}
		}
		attackLunge = false;

		Super::Jump(); //Inherits from APawn jump function
	}

}

void AFighterBase::StartRun()
{
	pbRunInput = true;
	GetWorldTimerManager().ClearTimer(PBInputTimerHandle);
	GetWorldTimerManager().SetTimer(PBInputTimerHandle, PBInputTimer, 0.25f, false); 
	if (pbBlockInput) //Checks for perfect block input
	{
		PerfectBlock();
	}
	else
	{
		isRunning = true;
		canRegenStamina = false;

		if (!isEvading)
		{
			GetCharacterMovement()->MaxWalkSpeed = RunningSpeed; //Starts run
		}
	}
}

void AFighterBase::EndRun()
{
	isRunning = false;
	if (!GetWorldTimerManager().IsTimerActive(staminaTimerHandle)) //Allows stamina regeneration if cooldown isn't active
	{
		canRegenStamina = true;
	}
	if (!isEvading && !isAttacking && !isFallingOver)
	{
		if (isTargetFocused || combatType == 3)
		{
			GetCharacterMovement()->MaxWalkSpeed = FocusedMovementSpeed; //Reverts to normal speeds
		}
		else
		{
			GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		}
	}
}

void AFighterBase::Evade()
{
	canRegenStamina = false; //Updates fighter state booleans
	isReactingToHit = false;
	canCombo = true;
	canBlock = true;
	canRecover = false;
	
	GetWorldTimerManager().ClearTimer(staminaTimerHandle);
	GetWorldTimerManager().SetTimer(staminaTimerHandle, regenerateStamina, 1.f, false);
	invincibilityHealth = 0.f;

	if (isAttacking) //Cancels attack before roll
	{
		isAttacking = false;
		canCombo = false;
		attackLunge = false;
	}
	SetAnimBlend(0.f);

	GetCharacterMovement()->MaxWalkSpeed = 650;
}

void AFighterBase::StartEvade() //Anim notifier
{
	isEvading = true;
}

void AFighterBase::FinishEvade() //Anim notifier
{
	isEvading = false;
	isMovingForward = false;
	isMovingBackwards = false;
	attackLunge = false;
	canCounter = false;

	if (isTargetFocused || combatType == 3)
	{
		SetUsingControllerRotation(true);
	}
	if (!isRunning && (isTargetFocused || combatType == 3)) //Reverts to normal speeds
	{
		GetCharacterMovement()->MaxWalkSpeed = FocusedMovementSpeed;
	}
	else if (isRunning)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
		return;
	}
	else if (isBlocking)
	{
		PlayAnimMontage(BlockAnim);
		GetCharacterMovement()->MaxWalkSpeed = BlockingMovementSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AFighterBase::CanBlock(bool result) //Anim notifier
{
	canBlock = result;
	if (isBlocking)
	{
		FinishHitReaction();
		Block();
	}
}

void AFighterBase::EvaluateAttackPosition()
{
	if (attackLunge && Target) //Moves actor towards target when attacking
	{
		FVector targetDirection = Target->GetActorLocation() - GetActorLocation();
		targetDirection.Z = 0;
		AddMovementInput(targetDirection * lungeDistance * GetWorld()->DeltaTimeSeconds);
	}
}

void AFighterBase::EvaluateAttackWindow()
{
	if (isAttacking && attackDealsDamage) //Starts logging the hit actors within the appropriate frames of the attack animation
	{
		TSet<AActor*> AttackHitActors;

		if (attackType == 4 && combatType == 1) //Only checks collision for the appropriate hitbox depending on the attack
		{
			RightElbowHitBox->GetOverlappingActors(AttackHitActors);
		}
		else if (attackType == 5)
		{
			RightFootHitBox->GetOverlappingActors(AttackHitActors);
		}
		else if (combatType == 1)
		{
			if ((attackType == 1 && (lightAttackIndex == 1 || lightAttackIndex == 3)) || (attackType == 2 && (heavyAttackIndex == 2)))
			{
				LeftHandHitBox->GetOverlappingActors(AttackHitActors);
			}
			else if ((attackType == 1 && (lightAttackIndex == 2 || lightAttackIndex == 0)) || (attackType == 2 && (heavyAttackIndex == 1 || heavyAttackIndex == 3)))
			{
				RightHandHitBox->GetOverlappingActors(AttackHitActors);
			}
			else if ((attackType == 2 && specialAttackIndex == 4))
			{
				LeftFootHitBox->GetOverlappingActors(AttackHitActors);
			}
			else if ((attackType == 2 && heavyAttackIndex == 0) || (attackType == 3 && (specialAttackIndex == 0 || specialAttackIndex == 1)))
			{
				RightFootHitBox->GetOverlappingActors(AttackHitActors);
			}
		}
		else if (combatType == 2)
		{
			MeleeWeapon->GetOverlappingActors(AttackHitActors); //Adds all actors that overlap with the player's weapon in to a set
		}

		for (AActor* hitActor : AttackHitActors) //Loops through all hit actors
		{
			if (hitActor && hitActor != this && !MeleeHitActors.Contains(hitActor)) //Conditions to prevent players from hurting themselves or duplicate hits
			{
				if (hitActor->GetClass()->IsChildOf<AProjectileBase>() || hitActor->GetClass()->IsChildOf<AWallBase>())
				{
					if (combatType == 2 || attackType == 3) //Allows fighters to destroy walls and projectiles
					{
						hitActor->Destroy();
						return;
					}
				}
				else
				{
					if (AEnemyBase* enemyTarget = Cast<AEnemyBase>(hitActor)) //Notifies AI fighters of the upcoming attack to allow them to generate a 'response'
					{
						enemyTarget->ReactToAttack(this, combatType, attackType);
					}

					AFighterBase* fighter = Cast<AFighterBase>(hitActor);
					if (fighter && fighter->GetPerfectBlockStatus()) //Stuns fighter if the hit actor was perfect blocking
					{
						invincibilityHealth = 0;
						isReactingToHit = true;
						attackDealsDamage = false;
						isAttacking = false;
						isEvading = false;
						canCombo = false;
						attackLunge = false;
						isMovingForward = false;
						isMovingBackwards = true;
						isBackGrounded = false;
						isFrontGrounded = false;
						canRegenStamina = true;
						canCounter = false;
						canBlock = false;

						SetAnimBlend(0.5f);
						GetCharacterMovement()->bOrientRotationToMovement = false;
						GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
						if (isTargetFocused || combatType == 3)
						{
							SetUsingControllerRotation(true);
						}
						stamina -= FMath::Min(stamina, 10.f);
						lightAttackIndex = 0;
						heavyAttackIndex = 0;

						hitReactionIndex = 4;
						PlayAnimMontage(HitReactions[hitReactionIndex]);
						return;
					}

					appliedDamage = UGameplayStatics::ApplyDamage(hitActor, attackDamage, GetController(), this, UDamageType::StaticClass()); //Damages actor
					
					if (fighter && appliedDamage > 0.f) //Only logs the hit if the hit actor wasn't evading
					{
						if (playerController) //Updates tutorial stats
						{
							if (attackType == 1)
							{
								playerController->UpdateLightAttacks();
							}
							else if (attackType == 2)
							{
								playerController->UpdateHeavyAttacks();
							}
							else if (attackType == 3)
							{
								playerController->UpdateSpecialAttacks();
							}
							else if (attackType == 4)
							{
								playerController->UpdateCounterAttacks();
							}

							if (combatType == 1)
							{
								playerController->UpdateKickboxingAttacks();
							}
							else if (combatType == 2)
							{
								playerController->UpdateWeaponAttacks();
							}
						}

						MeleeHitActors.Add(hitActor);

						if (Target == Cast<AFighterBase>(hitActor) && hitActor->IsA<AFighterBase>() && Cast<AFighterBase>(hitActor)->IsFighterDead())
						{
							Target = nullptr;
							SetTarget(false); //Updates target if hit actor is killed
						}
					}
				}

			}
		}
	}
}

void AFighterBase::ResetAttackIndex() //Called shortly after finishing an attack
{
	if (attackType == 1)
	{
		lightAttackIndex = 0;
	}
	if (attackType == 2)
	{
		heavyAttackIndex = 0;
	}
}

void AFighterBase::RegenerateStamina() //Called shortly after performing an action that consumes stamina
{
	if (!isRunning)
	{
		canRegenStamina = true;
	}
}
void AFighterBase::UpdatePerfectBlockStatus()
{
	isPerfectBlocking = false;
}
void AFighterBase::CanPerfectBlockCheck()
{
	if (!successfulPB) //Prevents fighter from perfect blocking for a second if perfect block was unsuccessful
	{
		StopBlocking();

		canRegenStamina = false;
		successfulPB = true;

		stamina -= FMath::Min(stamina, 10.f);
		
		GetWorldTimerManager().ClearTimer(canPBTimerHandle);
		GetWorldTimerManager().SetTimer(canPBTimerHandle, canPBTimer, 1.f, false);

		GetWorldTimerManager().ClearTimer(staminaTimerHandle);
		GetWorldTimerManager().SetTimer(staminaTimerHandle, regenerateStamina, 0.5f, false);
	}
	else
	{
		if (!isReactingToHit && !isEvading) //Allows players to perfect block again straight away if the previous one was successful
		{
			successfulPB = false;
			canPerfectBlock = true;
			canBlock = true;
		}
	}
}
void AFighterBase::ClearPerfectBlockInput() //If second input wasn't pressed in time
{
	pbBlockInput = false;
	pbRunInput = false;
}

//Handles health and current states when actor takes damage
float AFighterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (isEvading) //Evading prevents damage
	{
		if (playerController)
		{
			playerController->UpdateDodges();
		}

		return 0.f;
	}

	if (isBlocking && playerController)
	{
		playerController->UpdateBlocks();
	}

	damageRecieved = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser); //Gets applied damage from damage causer

	AFighterBase* FighterEnemy = Cast<AFighterBase>(DamageCauser);
	if (FighterEnemy)
	{
		enemyCombatType = FighterEnemy->GetCombatType(); //Stores what kind of attack the enemy used
		enemyAttackType = FighterEnemy->GetAttackType();
	}
	if (enemyAttackType == 5 || invincibilityHealth - damageRecieved <= 0.f) //Doesn't perform a hit reaction if the fighter tanks the attack
	{
		invincibilityHealth = 0.f;
		if (!isFallingOver && (!isFrontGrounded && !isBackGrounded))
		{
			StopAnimMontage(); //Stops all relevant states and anim montages when hit
			
			FVector targetDirection = DamageCauser->GetActorLocation() - GetActorLocation();
			float damageAngle = FVector::DotProduct(GetActorForwardVector(), targetDirection.GetSafeNormal());
			if (damageAngle < -0.3f) //Plays specific hit reaction if the player is hit from behind
			{
				hitReactionIndex = 3;
				
				isFallingOver = true;
				canBlock = false;
			}
			else
			{
				if (FighterEnemy) //Plays different hit reaction & sound depending on the type of attackand the defence used
				{
					if ((isBlocking && canBlock) && enemyAttackType == 5)
					{
						hitReactionIndex = 4;
						canBlock = false;
						UGameplayStatics::SpawnSoundAtLocation(this, BlockBreakerSFX, GetActorLocation());

						if (gameInstance && gameMode == 5 && playerController)
						{
							playerController->UpdateBlockBreakers();
						}
						
						stamina -= FMath::Min(stamina, 40.f);
					}
					else if ((isBlocking && canBlock) && ((enemyCombatType == 2 && enemyAttackType == 1) || (enemyCombatType == 1 && (enemyAttackType == 1 || enemyAttackType == 2)) || enemyAttackType == 4))
					{
						hitReactionIndex = -1;
						UGameplayStatics::SpawnSoundAtLocation(this, BlockSFX, GetActorLocation());
						canBlock = true;
					}
					else if ((isBlocking && canBlock) && ((enemyCombatType == 2 && enemyAttackType == 2) || enemyAttackType == 3))
					{
						hitReactionIndex = 1;
						canBlock = false;
						if (enemyCombatType == 1)
						{
							UGameplayStatics::SpawnSoundAtLocation(this, KickboxingSFX, GetActorLocation());
						}
						else
						{
							UGameplayStatics::SpawnSoundAtLocation(this, AxeSFX, GetActorLocation());
						}
					}

					else if ((isBlocking && canBlock) && (combatType == 1 || combatType == 3) && enemyCombatType == 2 && enemyAttackType == 2)
					{
						if (enemyCombatType == 1)
						{
							UGameplayStatics::SpawnSoundAtLocation(this, KickboxingSFX, GetActorLocation());
						}
						else
						{
							if (enemyAttackType == 3)
							{
								UGameplayStatics::SpawnSoundAtLocation(this, AxeSFX, GetActorLocation(), FRotator::ZeroRotator, 1.2f);
							}
							else
							{
								UGameplayStatics::SpawnSoundAtLocation(this, AxeSFX, GetActorLocation());
							}
						}
						int randNum = FMath::RandRange(0, 2);
						if (randNum == 0)
						{
							hitReactionIndex = 0;
						}
						else if (randNum == 1)
						{
							hitReactionIndex = 5;
						}
						else
						{
							hitReactionIndex = 6;
						}
						canBlock = false;
					}

					else
					{
						canBlock = false;
						if (enemyCombatType == 1)
						{
							UGameplayStatics::SpawnSoundAtLocation(this, KickboxingSFX, GetActorLocation());
						}
						else
						{
							UGameplayStatics::SpawnSoundAtLocation(this, AxeSFX, GetActorLocation());
						}
						
						if (enemyAttackType == 5 || enemyAttackType == 4 ||  (enemyCombatType == 2 && enemyAttackType == 1) || (enemyCombatType == 2 && enemyAttackType == 2) || (enemyCombatType == 1 && enemyAttackType == 3))
						{
							hitReactionIndex = 1;
						}
						else if ((enemyCombatType == 2 && enemyAttackType == 3))
						{
							hitReactionIndex = 2;
							isFallingOver = true;
						}
						else
						{
							int randNum = FMath::RandRange(0, 2);
							if (randNum == 0)
							{
								hitReactionIndex = 0;
							}
							else if (randNum == 1)
							{
								hitReactionIndex = 5;
							}
							else
							{
								hitReactionIndex = 6;
							}
						}
					}
				}

				else if (AProjectileBase* ProjectileEnemy = Cast<AProjectileBase>(DamageCauser)) //Hit reactions & sounds for projectile impact
				{
					UGameplayStatics::SpawnSoundAtLocation(this, KickboxingSFX, GetActorLocation());
					if ((isBlocking && canBlock) && damageRecieved == 50.f)
					{
						hitReactionIndex = 1;
						damageRecieved *= 0.5;
						canBlock = false;
					}
					else if ((isBlocking && canBlock) && damageRecieved == 25.f)
					{
						hitReactionIndex = -1;
						damageRecieved *= 0.7;
						canBlock = true;
					}
					else
					{
						canBlock = false;
						if (damageRecieved == 25.f)
						{
							hitReactionIndex = 1;
						}
						else if (damageRecieved == 50.f)
						{
							hitReactionIndex = 2;
							isFallingOver = true;
						}
					}
				}
			}

			isReactingToHit = true; //Updates fighter's state for hit reaction
			attackDealsDamage = false;
			isAttacking = false;
			isEvading = false;
			canCombo = false;
			attackLunge = false;
			
			isBackGrounded = false;
			isFrontGrounded = false;
			canRegenStamina = true;
			canCounter = false;
			canRecover = false;

			if (hitReactionIndex == 4)
			{
				SetAnimBlend(0.1f);
			}
			else
			{
				SetAnimBlend(0.f);
			}

			if (hitReactionIndex == 3) //Moves fighter forwards/backawards depending on the hit reaction
			{
				isMovingBackwards = false;
				isMovingForward = true;
				weightedInputDirection = GetActorForwardVector();
				PlayAnimMontage(HitReactions[hitReactionIndex]);
			}
			else
			{
				if (hitReactionIndex == -1)
				{
					PlayAnimMontage(BlockHitReaction);
				}
				else
				{
					PlayAnimMontage(HitReactions[hitReactionIndex]);
				}
				
				isMovingBackwards = true;
				isMovingForward = false;
				weightedInputDirection = -GetActorForwardVector();
			}

			GetCharacterMovement()->bOrientRotationToMovement = false;
			GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
			if (isTargetFocused || combatType == 3)
			{
				SetUsingControllerRotation(true);
			}
		}
		else if (isFrontGrounded || isBackGrounded) //Automatically gets the fighter up if damage taken whilst grounded
		{
			if (enemyCombatType == 1)
			{
				UGameplayStatics::SpawnSoundAtLocation(this, KickboxingSFX, GetActorLocation(), FRotator(0, 0, 0), 1.3f);
			}
			else
			{
				UGameplayStatics::SpawnSoundAtLocation(this, AxeSFX, GetActorLocation(), FRotator(0,0,0), 1.3f);
			}
			damageRecieved *= 1.2;
			if (isTargetFocused || combatType == 3)
			{
				SetUsingControllerRotation(true);
			}
			isEvading = true;
			isFallingOver = false;
			isReactingToHit = false;

			staminaDrained = FMath::Min(stamina, 40.f);
			stamina -= staminaDrained;

			if (isFrontGrounded)
			{
				GetCharacterMovement()->bOrientRotationToMovement = true;
				isFrontGrounded = false;
				isMovingForward = true;
				weightedInputDirection = GetActorForwardVector();

				PlayAnimMontage(RecoveryAnimations[2]);
			}
			else if (isBackGrounded)
			{
				isBackGrounded = false;
				isMovingBackwards = true;
				weightedInputDirection = -GetActorForwardVector();

				PlayAnimMontage(RecoveryAnimations[3]);
			}
			GetCharacterMovement()->MaxWalkSpeed = 650;
		}

		lightAttackIndex = 0;
		heavyAttackIndex = 0;
	}
	else if (invincibilityHealth - damageRecieved > 0.f) //Plays tank effect and sound if the invincibility health is greater than the damage recieved
	{
		ShowTankFX();
		UGameplayStatics::SpawnSoundAtLocation(this, TankSFX, GetActorLocation());
	}

	if (isAttacking)
	{
		damageRecieved *= 1.25;
	}

	damageRecieved = FMath::Min(health, damageRecieved); //Uses min so health never goes below 0
	health -= damageRecieved; //Applies damage

	if (health <= 0) //Checks for death
	{
		if (gameInstance && gameMode == 5 || gameMode == 6) //Doesn't kill in practice modes
		{
			health = maxHealth;
		}
		else
		{
			isDead = true;

			MeleeWeapon->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform); //Detaches melee weapon
			MeleeWeapon->SetSimulatePhysics(true);

			ACE301CapstoneProjectGameModeBase* capstoneGameMode = GetWorld()->GetAuthGameMode<ACE301CapstoneProjectGameModeBase>();
			if (capstoneGameMode)
			{
				capstoneGameMode->FighterKilled(this); //Logs fighter death to game mode handler class
			}

			ShowLockOnWidget(false, 1);
			ShowLockOnWidget(false, 2);
			GetMesh()->SetSimulatePhysics(true); //Enables ragdoll
			
			GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GetMesh()->AddRadialImpulse(DamageCauser->GetActorLocation(), 100, 1000, ERadialImpulseFalloff::RIF_Constant, true); //Applies force to ragdoll

			this->SetActorTickEnabled(false);
		}
	}

	return damageRecieved;
}

void AFighterBase::KickboxingCombatToggle() //Initiates kickboxing combat type
{
	if (!isReactingToHit && !isEvading && !isFrontGrounded && !isBackGrounded)
	{
		StopAttacking();

		if (combatType == 2)
		{
			PlayAnimMontage(UnequipAnimation);
			MeleeWeapon->SetHiddenInGame(true);
			SetAnimBlend(1.f);
		}

		if (combatType != 1)
		{
			combatType = 1;
			lightAttackIndex = 0;
			heavyAttackIndex = 0;
		}
	}
}

void AFighterBase::WeaponCombatToggle() //Weapon kickboxing combat type
{
	if (!isEvading && !isReactingToHit && !isFrontGrounded && !isBackGrounded)
	{
		StopAttacking();

		if (combatType != 2)
		{
			PlayAnimMontage(EquipAnimation);
			MeleeWeapon->SetHiddenInGame(false);
			combatType = 2;
			lightAttackIndex = 0;
			heavyAttackIndex = 0;
			SetAnimBlend(1.f);
		}
	}
}

void AFighterBase::MagicCombatToggle() //Magic kickboxing combat type
{
	if (!isReactingToHit && !isEvading && !isFrontGrounded && !isBackGrounded)
	{
		StopAttacking();

		if (combatType == 2)
		{
			PlayAnimMontage(UnequipAnimation);
			MeleeWeapon->SetHiddenInGame(true);
			SetAnimBlend(1.f);
		}

		if (combatType != 3)
		{
			lungeDistance = 0.f;
			combatType = 3;
			lightAttackIndex = 0;
			heavyAttackIndex = 0;
		}
	}
}


void AFighterBase::Attack() //Prepares attack variables and plays the appropriate animation
{
	GetWorldTimerManager().ClearTimer(attackIndexTimerHandle); //Prevents attack index from being reset
	GetWorldTimerManager().ClearTimer(staminaTimerHandle); //Prevents stamina from regenerating

	MeleeHitActors.Empty(); //Removes hit actors from previous attack

	isAttacking = true;
	attackDealsDamage = false;
	canCombo = false;
	canRegenStamina = false;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;

	if (attackType == 5) //Performs a block breaker
	{
		attackIndexResetTime = 0.25f;
		attackDamage = 10.f;
		lungeDistance = 300.f;
		if (BlockBreakAttack)
		{
			PlayAnimMontage(BlockBreakAttack);
		}
		SetAnimBlend(0.f);
	}
	else if (canCounter) //Performs a counter attack
	{
		canCounter = false;
		isEvading = false;
		attackIndexResetTime = 0.5f;
		invincibilityHealth = 34.f;
		
		
		specialAttackIndex = 1;
		lungeDistance = 600.f;
		attackDamage = counterDamage;
		SetAnimBlend(0);

		if (combatType == 2 && WeaponCounter)
		{
			lightAttackIndex = 1;
			heavyAttackIndex = 1;
			attackType = 4;
			PlayAnimMontage(WeaponCounter); //Plays the current attack animation
		}
		else if (combatType == 1 || combatType == 3 && KBCounter)
		{
			attackType = 4;
			lightAttackIndex = 2;
			heavyAttackIndex = 1;
			PlayAnimMontage(KBCounter); //Plays the current attack animation
		}
	}

	else if (combatType == 1) //For kickboxing atttacks
	{
		if (GetCharacterMovement()->IsFalling()) //Kickboxing air attack
		{
			attackIndexResetTime = 0.5f;
			SetAnimBlend(0.f);
			lungeDistance = 0.f;
			lightAttackIndex = 0;
			heavyAttackIndex = 0;
			specialAttackIndex = 4;
			attackType = 2;
			PlayAnimMontage(SpecialAttacks[4]);
			attackDamage = kickMediumDamage;
		}
		else if (attackType == 1) //Kickboxing light attack
		{
			attackIndexResetTime = 0.5f;
			lungeDistance = 600.f;
			attackDamage = kickLightDamage;
			SetAnimBlend(0.5f);
			if (KickboxingLightAttacks[lightAttackIndex])
			{
				PlayAnimMontage(KickboxingLightAttacks[lightAttackIndex]); //Plays the current attack animation
				lightAttackIndex == KickboxingLightAttacks.Num() - 1 ? lightAttackIndex = 0 :  lightAttackIndex++; //Selects next animation
			}
		}
		else if (attackType == 2) //Kickboxing heavy attack
		{
			lungeDistance = 800.f;
			attackIndexResetTime = 1.f;
			attackDamage = kickMediumDamage;
			if (isRunning)
			{
				SetAnimBlend(0.f);
				specialAttackIndex = 4;
				PlayAnimMontage(SpecialAttacks[specialAttackIndex]);
			}
			else
			{

				PlayAnimMontage(KickboxingHeavyAttacks[heavyAttackIndex]); //Plays the current attack animation
				SetAnimBlend(0.5f);

				if (heavyAttackIndex == KickboxingHeavyAttacks.Num() - 1)
				{
					heavyAttackIndex = 0; //Stops animation index from going out of range
					SetAnimBlend(0.f);
				}
				else
				{
					heavyAttackIndex++; //Selects next animation
				}
				
			}
		}
		else if (attackType == 3) //Kickboxing special attack
		{
			lungeDistance = 1000.f;
			attackIndexResetTime = 1.25f;
			attackDamage = kickSpecialDamage;
			if (isRunning)
			{
				specialAttackIndex = 0;
				PlayAnimMontage(SpecialAttacks[specialAttackIndex]);
				SetAnimBlend(0.f);
			}
			else
			{
				specialAttackIndex = 1;
				PlayAnimMontage(SpecialAttacks[specialAttackIndex]);
				SetAnimBlend(0.f);
			}
		}
	}

	else if (combatType == 2) //For weapon atttacks
	{
		if (GetCharacterMovement()->IsFalling()) //Weapon air attack
		{
			attackIndexResetTime = 0.5f;
			SetAnimBlend(0.1f);
			lungeDistance = 0.f;
			lightAttackIndex = 3;
			heavyAttackIndex = 0;
			specialAttackIndex = 0;
			attackType = 1;
			PlayAnimMontage(WeaponLightAttacks[3]);
			attackDamage = weaponMediumDamage;
		}
		else if (attackType == 1) //Weapon light attack
		{
			attackIndexResetTime = 1.f;
			attackDamage = weaponLightDamage;
			lungeDistance = 800.f;

			if (WeaponLightAttacks[lightAttackIndex])
			{
				PlayAnimMontage(WeaponLightAttacks[lightAttackIndex]); //Plays the current attack animation
			}

			SetAnimBlend(0.5f);
			lightAttackIndex == WeaponLightAttacks.Num() - 1 ? lightAttackIndex = 0 : lightAttackIndex++;
			
		}
		else if (attackType == 2) //Weapon heavy attack
		{
			attackIndexResetTime = 1.f;
			attackDamage = weaponMediumDamage;
			lungeDistance = 700.f;

			if (WeaponHeavyAttacks[heavyAttackIndex])
			{
				PlayAnimMontage(WeaponHeavyAttacks[heavyAttackIndex]); //Plays the current attack animation
			}
			
			if (GetCurrentMontage()->GetName() == "SwingUpAttack_Montage" || GetCurrentMontage()->GetName() == "360AttackLow_Montage")
			{
				SetAnimBlend(0.f);
			}
			else
			{
				SetAnimBlend(0.25f);
			}

			heavyAttackIndex == WeaponHeavyAttacks.Num() - 1 ? heavyAttackIndex = 0 : heavyAttackIndex++;
		}
		else if (attackType == 3) //Weapon special attack
		{
			SetAnimBlend(0.f);
			attackIndexResetTime = 1.5f;
			attackDamage = weaponSpecialDamage;
			specialAttackIndex = 2;
			lungeDistance = 1000.f;
			PlayAnimMontage(SpecialAttacks[specialAttackIndex]);
			GetCharacterMovement()->MaxWalkSpeed = RunningSpeed + 100;
		}
	}

	else if (combatType == 3) //For magic atttacks
	{
		SetAnimBlend(1.f);
		if (attackType == 1) //Light projectile
		{
			attackIndexResetTime = 0.75f;
			lightAttackIndex >= 1 ? lightAttackIndex = 0 : lightAttackIndex = 1;
			PlayAnimMontage(MagicLightAttacks[lightAttackIndex]);
		}
		else if (attackType == 2) //Heavy projectile
		{
			attackIndexResetTime = 1.f;
			PlayAnimMontage(MagicHeavyAttacks[0]);
		}
		else if (attackType == 3) //Defensive wall
		{
			SetAnimBlend(0.f);
			attackIndexResetTime = 1.5f;
			PlayAnimMontage(SpecialAttacks[3]);
		}
	}

	GetWorldTimerManager().SetTimer(staminaTimerHandle, regenerateStamina, 1.f, false); //Sets stamina regen cooldown after attacking

	if (isRunning)
	{
		EndRun();
	}
}

void AFighterBase::LightAttack() //Checks if fighter has enough stamina to perform a light attack
{
	if ((!isEvading || canCounter) && !isReactingToHit && !isFrontGrounded && !isBackGrounded && (!isAttacking || (isAttacking && canCombo)))
	{
		
		if (GetCharacterMovement()->IsFalling() && (combatType == 1 || combatType == 2) && stamina > 40.f)
		{
			staminaDrained = 40.f;
			invincibilityHealth = kickMediumDamage + 1;
		}
		else if (combatType == 1 && stamina > 10.f)
		{
			staminaDrained = 10.f;
			invincibilityHealth = 0;
		}
		else if (combatType == 2 && stamina > 40.f)
		{
			staminaDrained = 40.f;
			invincibilityHealth = 0;
		}
		else if (combatType == 3 && stamina > 30.f)
		{
			staminaDrained = 30.f;
			stamina -= staminaDrained;
			if (stamina < 0.f)
			{
				stamina = 0.f;
			}
			invincibilityHealth = 0;
		}
		else
		{
			return;
		}

		StopAnimMontage();
		attackType = 1;
		heavyAttackIndex = 0;

		Attack();
	}
}

void AFighterBase::HeavyAttack() //Checks if fighter has enough stamina to perform a heavy attack
{
	if ((!isEvading || canCounter) && !isReactingToHit && !isFrontGrounded && !isBackGrounded && (!isAttacking || (isAttacking && canCombo)))
	{
		
		if (GetCharacterMovement()->IsFalling() && (combatType == 1 || combatType == 2) && stamina > 40.f)
		{
			staminaDrained = 40.f;
			invincibilityHealth = 29.f;
		}
		if (combatType == 1 && stamina > 20.f)
		{
			staminaDrained = 20.f;
			invincibilityHealth = 0;

		}
		else if (combatType == 2 && stamina > 65.f)
		{
			staminaDrained = 65.f;
			invincibilityHealth = kickLightDamage + 1;
		}
		else if (combatType == 3 && stamina > 40.f)
		{
			staminaDrained = 40.f;
			stamina -= staminaDrained;
			if (stamina < 0.f)
			{
				stamina = 0.f;
			}
			invincibilityHealth = 0;
		}
		else
		{
			return;
		}

		StopAnimMontage();
		attackType = 2;
		lightAttackIndex = 0;

		Attack();
	}
}

void AFighterBase::SpecialAttack() //Checks if fighter has enough stamina to perform a special attack
{
	if ((!isEvading || canCounter) && !isReactingToHit && !isFrontGrounded && !isBackGrounded && (!isAttacking || (isAttacking && canCombo)))
	{
		
		if (combatType == 1 && stamina > 30.f)
		{
			staminaDrained = 30.f;
			invincibilityHealth = kickSpecialDamage + 1;
		}
		else if (combatType == 2 && stamina > 80.f)
		{
			staminaDrained = 40.f;
			invincibilityHealth = weaponMediumDamage + 1;
		}
		else if (combatType == 3 && stamina > 40.f)
		{
			staminaDrained = 40.f;
			stamina -= staminaDrained;
			if (stamina < 0.f)
			{
				stamina = 0.f;
			}
			invincibilityHealth = 0;
		}
		else
		{
			return;
		}
		
		StopAnimMontage();
		attackType = 3;
		lightAttackIndex = 0;
		heavyAttackIndex = 0;

		Attack();
	}
}

void AFighterBase::BlockBreaker()
{
	if (!isEvading && !isReactingToHit && !isFrontGrounded && !isBackGrounded && (!isAttacking || (isAttacking && canCombo)) && stamina > 30.f)
	{
		StopAttacking();
		StopAnimMontage();
		attackType = 5;
		staminaDrained = 40.f;

		Attack();
	}
}

void AFighterBase::SpawnMagicActor() //Spawns a projectile or wall depending on the fighter's input
{
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; //Spawn parameters determines how the engine will handle the actor once it's created
	spawnParams.Owner = this;
	spawnParams.Instigator = this;

	if (attackType == 1) //Creates a light projectile
	{
		AProjectileBase* Projectile = GetWorld()->SpawnActor<AProjectileBase>(LightProjectileClass, ProjectileSpawnPoint->GetComponentTransform(), spawnParams);
		UGameplayStatics::SpawnSoundAtLocation(this, FireLightSFX, GetActorLocation());
	}
	else if (attackType == 2) //Creates a heavy projectile
	{
		AProjectileBase* Projectile = GetWorld()->SpawnActor<AProjectileBase>(HeavyProjectileClass, ProjectileSpawnPoint->GetComponentTransform(), spawnParams);
		UGameplayStatics::SpawnSoundAtLocation(this, FireHeavySFX, GetActorLocation());
	}
	else if (attackType == 3 && !GetCharacterMovement()->IsFalling()) //Creates a defensie wall
	{
		FVector WallSpawnPoint = GetActorLocation() + (GetActorForwardVector() * 150.f);
		WallSpawnPoint.Z -= 425.f;
		AWallBase* NewWall = GetWorld()->SpawnActor<AWallBase>(WallBaseClass, WallSpawnPoint, GetActorRotation(), spawnParams);

		spawnedWalls.Enqueue(NewWall); //Keeps track of spawned walls and deletes oldest wall when limit reached
		if (++numWalls > 4)
		{
			AWallBase* deletedWall;
			spawnedWalls.Dequeue(deletedWall);
			deletedWall->Destroy();
		}
	}
}

void AFighterBase::Block() //Blocks attacks
{
	if ((!isReactingToHit || canBlock) && !isAttacking && !isEvading && !isFallingOver && !isFrontGrounded && !isBackGrounded)
	{
		EndRun();

		pbBlockInput = true;
		isBlocking = true;
		isMovingBackwards = false;
		
		if (pbRunInput)
		{
			PerfectBlock(); //Starts perfect block if the other required input is being used
		}
		else
		{
			SetAnimBlend(0.9f);
			FinishHitReaction();
			PlayAnimMontage(BlockAnim); //Plays block animation
		}

		GetCharacterMovement()->MaxWalkSpeed = BlockingMovementSpeed; //Slows movement

		GetWorldTimerManager().ClearTimer(PBInputTimerHandle);
		GetWorldTimerManager().SetTimer(PBInputTimerHandle, PBInputTimer, 0.25f, false);
	}
}

void AFighterBase::PerfectBlock()
{
	pbBlockInput = false;
	pbRunInput = false;
	if (canPerfectBlock) //Initiates perfect block
	{
		SetAnimBlend(0.1f);
		EndRun();
		FinishHitReaction();

		canPerfectBlock = false;
		canBlock = false;
		isPerfectBlocking = true;
		isAttacking = true;
		canCombo = false;
		stamina -= FMath::Min(stamina, 10.f);
		
		GetWorldTimerManager().ClearTimer(perfectBlockTimerHandle);
		GetWorldTimerManager().SetTimer(perfectBlockTimerHandle, perfectBlockTimer, 0.25f, false);
		
		PlayAnimMontage(PerfectBlockAnim);
		UGameplayStatics::SpawnSoundAtLocation(this, ForwardBackwardRollSFX, GetActorLocation(), FRotator::ZeroRotator, 0.8f);
	}
	GetWorldTimerManager().ClearTimer(canPBTimerHandle);
	GetWorldTimerManager().SetTimer(canPBTimerHandle, canPBTimer, 0.5f, false); //Reset the perfect block cooldown timer on every input
}

void AFighterBase::StopBlocking()
{
	if (isBlocking || GetCurrentMontage() == BlockAnim)
	{
		isBlocking = false;
		StopAnimMontage(BlockAnim);
		if (!isEvading)
		{
			if (!isTargetFocused)
			{
				GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
			}
			else
			{
				GetCharacterMovement()->MaxWalkSpeed = FocusedMovementSpeed;
			}
		}
	}
}

void AFighterBase::FinishHitReaction() //Allows actor to move again after being hit
{
	isReactingToHit = false;
	isMovingBackwards = false;
	isMovingForward = false;
	canCombo = true;
	canBlock = true;
	canPerfectBlock = true;
	isEvading = false;

	if (!isTargetFocused && combatType != 3)
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		SetUsingControllerRotation(false);
	}
	else
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->MaxWalkSpeed = FocusedMovementSpeed;
		SetUsingControllerRotation(true);
	}
	if (isRunning)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AFighterBase::SetIsGrounded(bool frontGrounded, bool backGrounded) //Anim notifier
{
	isFrontGrounded = frontGrounded;
	isBackGrounded = backGrounded;
}

void AFighterBase::SetIsFallingOver(bool result) //Anim notifier
{
	isFallingOver = result;
	if (result)
	{
		GetCharacterMovement()->MaxWalkSpeed = 600.f;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AFighterBase::CanLunge(bool can) //Anim notifier
{
	attackLunge = can;
}

void AFighterBase::StopAttacking() //At the end of the attack animation
{
	isAttacking = false;
	canCombo = false;
	attackLunge = false;

	if (isTargetFocused)
	{
		GetCharacterMovement()->MaxWalkSpeed = FocusedMovementSpeed;
	}

	GetWorldTimerManager().SetTimer(attackIndexTimerHandle, resetAttackIndex, attackIndexResetTime, false);
}

void AFighterBase::CanAttack() //Anim notifier
{
	canCombo = true;
}

void AFighterBase::CanCounter() //Anim notifier
{
	canCounter = true;
}

void AFighterBase::CanRecover(bool can) //Anim notifier
{
	canRecover = can;
}

void AFighterBase::SetMovingForward(bool movementInput) //Anim notifier
{
	isMovingForward = movementInput;
}

void AFighterBase::SetMovingBackwards(bool movementInput) //Anim notifier
{
	isMovingBackwards = movementInput;
}

void AFighterBase::SetAttackAppliesDamage(bool isDamaging) //Anim notifier
{
	attackDealsDamage = isDamaging;
	if (isDamaging)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, WhooshSFX, GetActorLocation());
		stamina -= staminaDrained;
		if (stamina < 0.f)
		{
			stamina = 0.f;
		}
	}
	else
	{
		invincibilityHealth = 0.f;
	}
}

float AFighterBase::GetHealth()
{
	return health;
}

float AFighterBase::GetStamina()
{
	return stamina;
}

float AFighterBase::GetHealthAmount()
{
	return health / maxHealth;
}

float AFighterBase::GetStaminaAmount()
{
	return stamina / maxStamina;
}

int AFighterBase::GetAttackType()
{
	return attackType;
}

int AFighterBase::GetCombatType()
{
	return combatType;
}

bool AFighterBase::GetPerfectBlockStatus() //Used to see if hit actor is perfect blocking
{
	if (isPerfectBlocking) //Log the perfect block as successful if called whilst performing a perfect block
	{
		successfulPB = true;
		FVector impactSpawn = GetActorLocation() + (GetActorForwardVector() * 25);
		impactSpawn.Z += 40;
		UNiagaraComponent* impact = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), perfectBlockVFX, impactSpawn);
		impact->SetVariableFloat("YawRotation", GetActorRotation().Yaw); //Spawns perfect block VFX

		UGameplayStatics::SpawnSoundAtLocation(this, PerfectBlockSFX, GetActorLocation());

		if (gameInstance && gameMode == 5 && playerController)
		{
			playerController->UpdatePerfectBlocks();
		}
	}

	return isPerfectBlocking;
}

bool AFighterBase::IsFighterDead()
{
	return isDead;
}

void AFighterBase::ShowLockOnWidget(bool isVisible, int playerIndex)
{
	if (playerIndex == 1)
	{
		LockOnWidgetP1->SetVisibility(isVisible);
	}
	else if (playerIndex == 2)
	{
		LockOnWidgetP2->SetVisibility(isVisible);
	}
}

AActor* AFighterBase::GetFighterTarget()
{
	return Target;
}

void AFighterBase::SetTarget(AActor* fighterTarget)
{
	Target = fighterTarget;
	if (Target == nullptr && isTargetFocused)
	{
		isTargetFocused = false;
	}
}

void AFighterBase::Taunt() //Plays taunt animation
{
	if (!isReactingToHit && !isAttacking && !isEvading && !isFallingOver && !isFrontGrounded && !isBackGrounded && !GetCharacterMovement()->IsFalling())
	{
		isReactingToHit = true;

		SetAnimBlend(0.f);
		PlayAnimMontage(TauntAnimations[tauntIndex]);

		tauntIndex++;
		if (tauntIndex > 1)
		{
			tauntIndex = 0;
		}
	}
}

void AFighterBase::SetUsingControllerRotation(bool condition)
{
	if (!autoTurn)
	{
		bUseControllerRotationYaw = condition; //Sets the player's rotation relative the camera rotation
		bUseControllerRotationRoll = condition;
	}
}

void AFighterBase::SetAnimBlend(float newBlend)
{
	animBlend = newBlend;
}

void AFighterBase::PrintStates()
{
	UE_LOG(LogTemp, Warning, TEXT("CombatType: %i"), combatType);

	UE_LOG(LogTemp, Warning, TEXT("AttackType: %i"), attackType);

	if (isAttacking)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attacking: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Attacking: false"));
	}

	if (attackLunge)
	{
		UE_LOG(LogTemp, Warning, TEXT("Lunging: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Lunging: false"));
	}

	if (attackDealsDamage)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackDamaging: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackDamaging: false"));
	}

	if (canCombo)
	{
		UE_LOG(LogTemp, Warning, TEXT("NextAttackReady: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NextAttackReady: false"));
	}

	if (isBlocking)
	{
		UE_LOG(LogTemp, Warning, TEXT("Blocking: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Blocking: false"));
	}

	if (isMovingForward)
	{
		UE_LOG(LogTemp, Warning, TEXT("MovingForward: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MovingForward: false"));
	}

	if (isMovingBackwards)
	{
		UE_LOG(LogTemp, Warning, TEXT("MovingBackwards: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MovingBackwards: false"));
	}

	if (isReactingToHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stumbling: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Stumbling: false"));
	}

	if (isTargetFocused)
	{
		UE_LOG(LogTemp, Warning, TEXT("Target Focused: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Target Focused: false"));
	}
	if (canRegenStamina)
	{
		UE_LOG(LogTemp, Warning, TEXT("Regen Stamina: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Regen Stamina: false"));
	}
	if (isEvading)
	{
		UE_LOG(LogTemp, Warning, TEXT("Rolling: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Rolling: false"));
	}

	if (canPerfectBlock)
	{
		UE_LOG(LogTemp, Warning, TEXT("canPerfectBlock: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("canPerfectBlock: false"));
	}
	if (canBlock)
	{
		UE_LOG(LogTemp, Warning, TEXT("canBlock: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("canBlock: false"));
	}
	if (successfulPB)
	{
		UE_LOG(LogTemp, Warning, TEXT("successfulPB: true"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("successfulPB: false"));
	}
}
