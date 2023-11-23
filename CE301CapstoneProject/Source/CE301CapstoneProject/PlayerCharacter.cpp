// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "GameFramework/Actor.h"
#include "EnemyBase.h"
#include "PlayerFighterController.h"
#include "CapstoneGameInstance.h"
#include "TutorialBox.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "GameFramework/Controller.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/CameraShake.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "Blueprint/UserWidget.h"
#include "CE301CapstoneProjectGameModeBase.h"
#include "EngineUtils.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(50.0f, 90.0f);

	PlayerSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm")); //Attaches spring arm to root
	PlayerSpringArm->SetupAttachment(RootComponent);
	PlayerSpringArm->TargetArmLength = 415.0f; 
	PlayerSpringArm->SocketOffset = { 0.f,35.f,40.f };
	PlayerSpringArm->bUsePawnControlRotation = true; 

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera")); //Attaches camera to spring arm
	PlayerCamera->SetupAttachment(PlayerSpringArm, USpringArmComponent::SocketName);
	PlayerCamera->bUsePawnControlRotation = false; //Camera's rotation is unlinked from the player

	ActorCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Actor Collision Sphere"));
	ActorCollisionSphere->SetupAttachment(RootComponent);
	ActorCollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ActorCollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap); // Only allows overlap overlaps with type pawn
	ActorCollisionSphere->SetSphereRadius(enemyLockRange);

	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Spawn Point"));
	ProjectileSpawnPoint->SetupAttachment(PlayerSpringArm);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 400.0f;
	GetCharacterMovement()->AirControl = 0.3f;

	autoTurn = false;
	SetUsingControllerRotation(false);
	

	const wchar_t* akaiMeshPath = TEXT("SkeletalMesh'/Game/Assets/Actors/Akai/akai_e_espiritu.akai_e_espiritu'"); //Stores the models and materials for the 2 different player characters
	auto akaiMeshReference = ConstructorHelpers::FObjectFinder<USkeletalMesh>(akaiMeshPath);
	akaiMesh = akaiMeshReference.Object;

	const wchar_t* akaiMaterialPath = TEXT("Material'/Game/Assets/Actors/Akai/Akai_MAT.Akai_MAT'");
	auto akaiMaterialReference = ConstructorHelpers::FObjectFinder<UMaterial>(akaiMaterialPath);
	akaiMaterial = akaiMaterialReference.Object;

	const wchar_t* ganfaulMeshPath = TEXT("SkeletalMesh'/Game/Assets/Actors/Ganfaul/ganfaul_edit_4.ganfaul_edit_4'");
	auto ganfaulMeshReference = ConstructorHelpers::FObjectFinder<USkeletalMesh>(ganfaulMeshPath);
	ganfaulMesh = ganfaulMeshReference.Object;

	const wchar_t* ganfaulMaterialPath = TEXT("Material'/Game/Assets/Actors/Ganfaul/Ganfaul_Body.Ganfaul_Body'");
	auto ganfaulMaterialReference = ConstructorHelpers::FObjectFinder<UMaterial>(ganfaulMaterialPath);
	ganfaulMaterial = ganfaulMaterialReference.Object;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	MeleeWeapon->SetHiddenInGame(true);

	if (this == UGameplayStatics::GetPlayerCharacter(this, 0)) //Changes the model depending on the player's selection in the menus
	{
		playerIndex = 1;
		if (gameInstance && gameInstance->GetPlayer1Mesh() == 0)
		{
			GetMesh()->SetSkeletalMesh(akaiMesh);
			GetMesh()->SetMaterial(0, akaiMaterial);
		}
		else
		{
			GetMesh()->SetSkeletalMesh(ganfaulMesh);
			GetMesh()->SetMaterial(0, ganfaulMaterial);
		}
	}
	else
	{
		playerIndex = 2;

		if (gameInstance && gameInstance->GetPlayer2Mesh() == 0)
		{
			GetMesh()->SetSkeletalMesh(akaiMesh);
			GetMesh()->SetMaterial(0, akaiMaterial);
		}
		else
		{
			GetMesh()->SetSkeletalMesh(ganfaulMesh);
			GetMesh()->SetMaterial(0, ganfaulMaterial);
		}

		enemyDetectionTimer = FTimerDelegate::CreateUObject(this, &APlayerCharacter::DetectEnemiesAtSpawn);
		GetWorldTimerManager().SetTimer(enemyDetectionTimerHandle, enemyDetectionTimer, 0.25f, false);
	}

	CreateDynamicMaterialInstances(); 

	weightedInputDirection = GetActorForwardVector();

	ActorCollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &APlayerCharacter::TriggerActorOverlapInRadius); //Triggers functions whenever actors enter/leave the collision sphere
	ActorCollisionSphere->OnComponentEndOverlap.AddDynamic(this, &APlayerCharacter::EndActorOverlapInRadius);
	ActorCollisionSphere->GetOverlappingActors(ActorsInRange);
	for (AActor* currentActor : ActorsInRange)
	{
		if (currentActor != this && currentActor->GetClass()->IsChildOf<AEnemyBase>())
		{
			EnemiesInRange.Add(currentActor);
		}
		else if ((gameMode != 1 && gameMode != 4) && currentActor != this && currentActor->GetClass()->IsChildOf<AFighterBase>())
		{
			EnemiesInRange.Add(currentActor);
		}
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RefreshLockedTarget();

	DisplayDebugInfo();

	if (Target && isTargetFocused) //Rotates the player camera if a target is in range and the player's locked on to them
	{
		FVector targetDirection = Target->GetActorLocation() - GetActorLocation();
		if (targetDirection.Size2D() > 10.f) //10.f gives slight leeway at target's rotation
		{
			FRotator cameraDifference = UKismetMathLibrary::NormalizedDeltaRotator(Controller->GetControlRotation(), targetDirection.ToOrientationRotator());
			AddControllerYawInput((-cameraDifference.Yaw - 2.f) * 0.9 * DeltaTime); //Adjusts camera to target's position
		}
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) //Binds player input to functions
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerCharacter::MoveForward); //Binds movement inputs
	PlayerInputComponent->BindAxis("MoveHorizontal", this, &APlayerCharacter::MoveHorizontal);
	PlayerInputComponent->BindAxis("LookVertical", this, &APlayerCharacter::RotateCameraVertical);
	PlayerInputComponent->BindAxis("LookHorizontal", this, &APlayerCharacter::RotateCameraHorizontal);
	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &APlayerCharacter::StartRun);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &APlayerCharacter::EndRun);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APlayerCharacter::Jump);

	PlayerInputComponent->BindAction("LightAttack", IE_Pressed, this, &APlayerCharacter::LightAttack); //Binds combat inputs
	PlayerInputComponent->BindAction("HeavyAttack", IE_Pressed, this, &APlayerCharacter::HeavyAttack);
	PlayerInputComponent->BindAction("SpecialAttack", IE_Pressed, this, &APlayerCharacter::SpecialAttack);
	PlayerInputComponent->BindAction("BlockBreaker", IE_Pressed, this, &APlayerCharacter::BlockBreaker);
	PlayerInputComponent->BindAction("Evade", IE_Pressed, this, &APlayerCharacter::Evade);
	PlayerInputComponent->BindAction("Block", IE_Pressed, this, &APlayerCharacter::Block);
	PlayerInputComponent->BindAction("Block", IE_Released, this, &APlayerCharacter::StopBlocking);

	PlayerInputComponent->BindAction("ToggleEnemyLock", IE_Pressed, this, &APlayerCharacter::ToggleEnemyLock); //Binds target lock inputs
	PlayerInputComponent->BindAction("CycleTargetRight", IE_Pressed, this, &APlayerCharacter::CycleTargetRight);
	PlayerInputComponent->BindAction("CycleTargetLeft", IE_Pressed, this, &APlayerCharacter::CycleTargetLeft);

	PlayerInputComponent->BindAction("KickboxingCombatToggle", IE_Pressed, this, &APlayerCharacter::KickboxingCombatToggle);
	PlayerInputComponent->BindAction("WeaponCombatToggle", IE_Pressed, this, &APlayerCharacter::WeaponCombatToggle);
	PlayerInputComponent->BindAction("MagicCombatToggle", IE_Pressed, this, &APlayerCharacter::MagicCombatToggle);

	PlayerInputComponent->BindAction("Taunt", IE_Pressed, this, &APlayerCharacter::Taunt);
	PlayerInputComponent->BindAction("ActivateTutorialBox", IE_Pressed, this, &APlayerCharacter::ActivateTutorialBox);

	PlayerInputComponent->BindAction("TogglePlayerDebug", IE_Pressed, this, &APlayerCharacter::TogglePlayerDebug);
	PlayerInputComponent->BindAction("ToggleEnemyDebug", IE_Pressed, this, &APlayerCharacter::ToggleEnemyDebug);

	FInputActionBinding& toggle = PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &APlayerCharacter::PauseInput);
	toggle.bExecuteWhenPaused = true;
}

void APlayerCharacter::PauseInput()
{
	if (playerController)
	{
		if (playerController->IsPaused())
		{
			playerController->TogglePaused(false);
		}
		else
		{
			playerController->TogglePaused(true);
			
		}
	}
}

void APlayerCharacter::EvaluateAttackWindow()
{
	Super::EvaluateAttackWindow();

	if (appliedDamage > 0.f)///Only stops duplication and allows camera shake if the actor isn't dead 
	{
		UGameplayStatics::GetPlayerController(this, playerIndex-1)->PlayerCameraManager->StartCameraShake(CameraShakeOnHit);
		appliedDamage = 0.f;
	}
}

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser); //Inherits from AFighterBase's function

	if (IsFighterDead())
	{
		RemoveHUD();
		PlayerSpringArm->bDoCollisionTest = false;
		PlayerSpringArm->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::KeepWorld, false)); //Adjusts camera after death
		PlayerSpringArm->SocketOffset = { 0.f,35.f,200.f };
		PlayerCamera->SetRelativeRotation(FRotator(340, 0, 0));
	}

	return damageRecieved;
}

void APlayerCharacter::RotateCameraHorizontal(float value) //Rotates the camera based on the mouse's Y axis input
{
	AddControllerYawInput(value * CameraHorizontalSensitivity * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::RotateCameraVertical(float value) //Rotates the camera based on the mouse's X axis input
{
	AddControllerPitchInput(value * CameraVerticalSensitivity * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::MoveForward(float value)
{
	const FRotator playerYaw(0.f, Controller->GetControlRotation().Yaw, 0.f); //Allows the player to move relative to the camera rotation
	const FVector forwardDirection = FRotationMatrix(playerYaw).GetUnitAxis(EAxis::X); //Adjusts the player's forward vector to rely on the camera's rotation
	weightedForwardInput = forwardDirection;
	if (Controller && (value != 0.f) && (!isReactingToHit || (isReactingToHit && isBlocking)) && !isFrontGrounded && !isBackGrounded && !isEvading)
	{
		if (isAttacking && combatType != 3)
		{
			AddMovementInput(forwardDirection * (value * 0.5)); //Slows down the player if they're attacking
		}
		else
		{
			AddMovementInput(forwardDirection * value);
		}
	}
	weightedForwardInput *= value;
	inputWeight.X = value;

}

void APlayerCharacter::MoveHorizontal(float value)
{
	const FRotator playerYaw(0.f, Controller->GetControlRotation().Yaw, 0.f); //Allows the player to move relative to the camera rotation
	const FVector horizontalDirection = FRotationMatrix(playerYaw).GetUnitAxis(EAxis::Y); //Adjusts the player's sideways movement vector to rely on the camera's rotation
	weightedHorizontalInput = horizontalDirection;
	if (Controller && (value != 0.f) && !isReactingToHit&& !isFrontGrounded && !isBackGrounded && !isEvading)
	{
		if (isAttacking && combatType != 3)
		{
			AddMovementInput(horizontalDirection * (value * 0.5)); //Slows down the player if they're attacking
		}
		else
		{
			AddMovementInput(horizontalDirection * value);
		}
	}
	weightedHorizontalInput *= value;
	inputWeight.Y = value;
}

void APlayerCharacter::Evade() //Handles the player's evade input
{
	if (!isEvading && (!isReactingToHit || (isReactingToHit && isBlocking && canBlock) || canBlock) && !isFallingOver && stamina > 20.f)
	{
		Super::Evade();
		
		weightedInputDirection = weightedForwardInput + weightedHorizontalInput;
		if (inputWeight.X < 0 && FMath::Abs(inputWeight.Y) < 0.6 && FMath::Abs(weightedInputDirection.Y) < 180 && (isTargetFocused || combatType == 3)) //If the player's input is pointing backwards
		{
			weightedInputDirection = -GetActorForwardVector();
			if (GetCharacterMovement()->IsFalling()) //Plays back evade air animation
			{
				PlayAnimMontage(EvadeAnimations[4]);
				staminaDrained = FMath::Min(stamina, airEvadeStaminaCost);
				UGameplayStatics::SpawnSoundAtLocation(this, WhooshSFX, GetActorLocation());
			}
			else //Grounded back evade
			{
				PlayAnimMontage(EvadeAnimations[2]);
				staminaDrained = FMath::Min(stamina, runEvadeStaminaCost);
				UGameplayStatics::SpawnSoundAtLocation(this, ForwardBackwardRollSFX, GetActorLocation()); //Use the forward 
			}
			weightedInputDirection = -GetActorForwardVector();
			isMovingBackwards = true;
		}
		else if (inputWeight.X == 0 && inputWeight.Y == 0) //No movement input
		{
			isEvading = true;
			staminaDrained = FMath::Min(stamina, standingEvadeStaminaCost);

			if (evadeIndex == 0)
			{
				evadeIndex = 1;
				PlayAnimMontage(EvadeAnimations[3]); //Plays alternaing standing evade
			}
			else
			{
				evadeIndex = 0;
				PlayAnimMontage(EvadeAnimations[7]);
			}
			UGameplayStatics::SpawnSoundAtLocation(this, WhooshSFX, GetActorLocation());
		}
		else if (GetCharacterMovement()->IsFalling())
		{
			if ((inputWeight.X > -0.3f && inputWeight.X < 0.3f) && FMath::Abs(inputWeight.Y) > 0.3 && (isTargetFocused || combatType == 3)) //Horizontal air evade
			{
				if (inputWeight.Y > 0)
				{
					PlayAnimMontage(EvadeAnimations[9]);
				}
				else
				{
					PlayAnimMontage(EvadeAnimations[8]);
				}

				UGameplayStatics::SpawnSoundAtLocation(this, WhooshSFX, GetActorLocation());
			}
			else //Forward air evade
			{
				PlayAnimMontage(EvadeAnimations[1]);
			}
			staminaDrained = FMath::Min(stamina, airEvadeStaminaCost);
		}
		else if (inputWeight.X >= 0.1 || (!isTargetFocused && combatType != 3)) //Grounded forward evade
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
			if (isTargetFocused || combatType == 3)
			{
				SetUsingControllerRotation(false);
			}
			PlayAnimMontage(EvadeAnimations[0]);
			staminaDrained = FMath::Min(stamina, runEvadeStaminaCost);
			UGameplayStatics::SpawnSoundAtLocation(this, WhooshSFX, GetActorLocation());
		}
		else //Horizontal ground evade
		{
			isMovingForward = true;
			isEvading = true;
			staminaDrained = FMath::Min(stamina, runEvadeStaminaCost);
			if (inputWeight.Y > 0)
			{
				PlayAnimMontage(EvadeAnimations[5]);
			}
			else
			{
				PlayAnimMontage(EvadeAnimations[6]);
			}

			UGameplayStatics::SpawnSoundAtLocation(this, SideRollSFX, GetActorLocation());
		}
		stamina -= staminaDrained;
	}
	else if (isFallingOver && canRecover && stamina > 40.f)
	{
		Super::Evade();
		
		if (isTargetFocused || combatType == 3)
		{
			SetUsingControllerRotation(false);
		}
		isEvading = true;
		isFallingOver = false;

		staminaDrained = FMath::Min(stamina, recoverStaminaCost);
		stamina -= staminaDrained;

		SetAnimBlend(0);

		if (isMovingForward)
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
			weightedInputDirection = GetActorForwardVector();
			PlayAnimMontage(RecoveryAnimations[2]);
		}
		else if (isMovingBackwards)
		{
			weightedInputDirection = -GetActorForwardVector();
			PlayAnimMontage(RecoveryAnimations[3]);
		}
	}
	else if (isFrontGrounded)
	{
		PlayAnimMontage(RecoveryAnimations[0]);
		isFrontGrounded = false;
	}
	else if (isBackGrounded)
	{
		PlayAnimMontage(RecoveryAnimations[1]);
		isBackGrounded = false;
	}
}

void APlayerCharacter::KickboxingCombatToggle()
{
	if (!isReactingToHit && !isFrontGrounded && !isBackGrounded && !isEvading)
	{
		if (combatType != 1 && !isTargetFocused)
		{
			SetUsingControllerRotation(false);
			GetCharacterMovement()->bOrientRotationToMovement = true;
			if (!isRunning)
			{
				GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
			}
		}

		attackLunge = true;
		Super::KickboxingCombatToggle();
	}
}

void APlayerCharacter::WeaponCombatToggle()
{
	if (!isReactingToHit && !isFrontGrounded && !isBackGrounded && !isEvading)
	{
		if (combatType != 2 && !isTargetFocused)
		{
			SetUsingControllerRotation(false);
			GetCharacterMovement()->bOrientRotationToMovement = false;
			if (!isRunning)
			{
				GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
			}
		}

		attackLunge = false;
		Super::WeaponCombatToggle();
	}
}

void APlayerCharacter::MagicCombatToggle()
{
	
	if (!isReactingToHit && !isFallingOver && !isFrontGrounded && !isBackGrounded && !isEvading)
	{
		if (!isRunning)
		{
			GetCharacterMovement()->MaxWalkSpeed = FocusedMovementSpeed; //Slows the player if lock on is successful
		}
		GetCharacterMovement()->bOrientRotationToMovement = false;
		SetUsingControllerRotation(true);

		attackLunge = false;
		Super::MagicCombatToggle();
	}
}

void APlayerCharacter::Attack()
{
	if (!isTargetFocused) //Finds closest enemy to input direction for tracking
	{
		Target = UpdateNearEnemies();
	}

	Super::Attack();

	if (inputWeight.X == 0 && inputWeight.Y == 0)
	{
		lungeDistance = 50.f;
	}
}

void APlayerCharacter::StopAttacking() //Linked to anim notifier at the end of an attack
{
	Super::StopAttacking();
}

void APlayerCharacter::RefreshLockedTarget()
{
	if (Target)
	{
		if (FVector::Distance(Target->GetActorLocation(), GetActorLocation()) > enemyLockRange || !Target) //Removes target if they're out of the sphere's range
		{
			if (AFighterBase* FighterTarget = Cast<AFighterBase>(Target))
			{
				FighterTarget->ShowLockOnWidget(false, playerIndex);
			}

			Target = nullptr; //Disables enemy lock
			isTargetFocused = false;
			if (combatType != 3)
			{
				if (!isRunning && !isEvading)
				{
					GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
				}
				GetCharacterMovement()->bOrientRotationToMovement = true;
				SetUsingControllerRotation(false);
			}
		}
	}
}

void APlayerCharacter::SetTarget(AActor* fighterTarget)
{
	Super::SetTarget(fighterTarget);

	if (Target == nullptr) //Disables enemy lock if target is removed
	{
		if (AFighterBase* FighterTarget = Cast<AFighterBase>(Target))
		{
			FighterTarget->ShowLockOnWidget(false, playerIndex);

		}

		isTargetFocused = false;
		if (combatType != 3)
		{
			if (!isRunning && !isEvading)
			{
				GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
			}
			GetCharacterMovement()->bOrientRotationToMovement = true;
			SetUsingControllerRotation(false);
		}
	}
}

void APlayerCharacter::FinishHitReaction()
{
	Super::FinishHitReaction();

	if (isBlocking)
	{
		SetAnimBlend(0.9f);
		GetCharacterMovement()->MaxWalkSpeed = BlockingMovementSpeed;
		PlayAnimMontage(BlockAnim);
	}
}

void APlayerCharacter::CycleTarget(bool Clockwise)
{
	UpdateNearEnemies();
	if (EnemiesInRange.Num() > 1)
	{
		float nearestRightEnemy = -180.f; //floats for determining the nearest enemy on either side of the current target
		float nearestLeftEnemy = 180.f;

		AActor* tempEnemy = Target; //Temporary target whilst all options are filtered
		FVector targetDirection = Target->GetActorLocation() - GetActorLocation();
		FRotator targetDifference = UKismetMathLibrary::NormalizedDeltaRotator(Controller->GetControlRotation(), targetDirection.ToOrientationRotator());
		float targetYaw = targetDifference.Yaw; //Gets the difference in yaw between the player and the current target

		for (AActor* nearEnemy : EnemiesInRange)
		{
			FVector EnemyDirection = nearEnemy->GetActorLocation() - GetActorLocation();
			FRotator EnemyDifference = UKismetMathLibrary::NormalizedDeltaRotator(Controller->GetControlRotation(), EnemyDirection.ToOrientationRotator());
			float enemyYaw = EnemyDifference.Yaw; //Gets the difference in yaw between the player and an enemy in range

			if ((Clockwise && enemyYaw > targetYaw) || (!Clockwise && enemyYaw < targetYaw)) //Continue if the cycle input does not match the desired direction 
			{
				continue;
			}

			if ((Clockwise && enemyYaw < targetYaw) && enemyYaw > nearestRightEnemy) //True if the cycle input matches the desired direction and the enemy is closer to the target than the current nearest
			{
				nearestRightEnemy = enemyYaw;
				tempEnemy = nearEnemy;
			}
			if ((!Clockwise && enemyYaw > targetYaw) && enemyYaw < nearestLeftEnemy)
			{
				nearestLeftEnemy = enemyYaw;
				tempEnemy = nearEnemy;
			}
		}

		if (tempEnemy != Target)
		{
			if (AFighterBase* FighterTarget = Cast<AFighterBase>(Target))
			{
				FighterTarget->ShowLockOnWidget(false, playerIndex);
			}
		}

		Target = tempEnemy; //Assigns the target as the nearest enemy in the correct direction

		if (AFighterBase* FighterTarget = Cast<AFighterBase>(Target))
		{
			FighterTarget->ShowLockOnWidget(true, playerIndex);
		}
	}

}

void APlayerCharacter::ActivateTutorialBox() //Displays dialogue text after interacting with box
{
	if (gameMode == 5)
	{
		TSet<UPrimitiveComponent*> ov;
		GetOverlappingComponents(ov);
		for (UPrimitiveComponent* tBox : ov)
		{
			
			if (tBox->ComponentHasTag("tbox") && playerController)
			{
				if (ATutorialBox* boxOwner = Cast<ATutorialBox>(tBox->GetOwner()))
				{
					playerController->ActivateTutorialHUD(boxOwner->GetBoxType());
					break;
				}
			}
		}
	}
}
void APlayerCharacter::CycleTargetRight()
{
	if (Target && isTargetFocused)
	{
		CycleTarget(true);
	}
}

void APlayerCharacter::CycleTargetLeft()
{
	if (Target && isTargetFocused)
	{
		CycleTarget(false);
	}
}

void APlayerCharacter::ToggleEnemyLock()
{
	if (!isTargetFocused)
	{
		Target = UpdateNearEnemies();
		if (Target) //Updates enemies in range and checks if the nearest target is not null
		{
			isTargetFocused = true;
			GetCharacterMovement()->bOrientRotationToMovement = false;
			if (AFighterBase* FighterTarget = Cast<AFighterBase>(Target))
			{
				FighterTarget->ShowLockOnWidget(true, playerIndex);
			}

			if (!isAttacking && !isEvading && !isFallingOver && combatType != 3)
			{
				if (!isRunning)
				{
					GetCharacterMovement()->MaxWalkSpeed = FocusedMovementSpeed; //Slows the player if lock on is successful
				}
				GetCharacterMovement()->bOrientRotationToMovement = false;
				SetUsingControllerRotation(true);
			}
			
		}
	}
	else //Disables enemy lock
	{
		isTargetFocused = false;
		if (AFighterBase* FighterTarget = Cast<AFighterBase>(Target))
		{
			FighterTarget->ShowLockOnWidget(false, playerIndex);
		}

		if (!isAttacking && !isEvading && !isFallingOver && combatType != 3)
		{
			if (!isRunning)
			{
				GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
			}
			GetCharacterMovement()->bOrientRotationToMovement = true;
			SetUsingControllerRotation(false);
		}
	}
}

void APlayerCharacter::DetectEnemiesAtSpawn() //Gets enemies in radius at spawn
{
	ActorCollisionSphere->GetOverlappingActors(ActorsInRange);
	for (AActor* currentActor : ActorsInRange)
	{
		if (currentActor != this && currentActor->GetClass()->IsChildOf<AEnemyBase>())
		{
			EnemiesInRange.Add(currentActor);
		}
		else if ((gameMode != 1 && gameMode != 4) && currentActor != this && currentActor->GetClass()->IsChildOf<AFighterBase>())
		{
			EnemiesInRange.Add(currentActor);
		}
	}
}

AActor* APlayerCharacter::UpdateNearEnemies() //Called when enemies in range needs to be refreshed
{
	AActor* tempTarget = Target;
	float nearestEnemyDistance = enemyLockRange;
	float nearestEnemyYaw = -1.f;
	
	for (AActor* currentEnemy : EnemiesInRange)
	{
		if (inputWeight.X == 0 && inputWeight.Y == 0) //Gets the closest actor if the player isn't moving
		{
			float currentEnemyDistance = FVector::Distance(currentEnemy->GetActorLocation(), GetActorLocation());

			if (currentEnemyDistance < nearestEnemyDistance) //If the actor in loop is closer than the current nearest enemy
			{
				nearestEnemyDistance = currentEnemyDistance;
				tempTarget = currentEnemy;
			}
		}
		else //Gets the actor closest to the input direction
		{
			FVector targetDirection1 = currentEnemy->GetActorLocation() - GetActorLocation();
			FVector weightedRotation1 = weightedForwardInput + weightedHorizontalInput;

			weightedRotation1.Normalize();
			targetDirection1.Normalize();
			float targetYaw1 = FVector::DotProduct(weightedRotation1, targetDirection1);
			if (targetYaw1 > nearestEnemyYaw)
			{
				tempTarget = currentEnemy;
				nearestEnemyYaw = targetYaw1;
			}

		}
	}
	return tempTarget;
}


void APlayerCharacter::TriggerActorOverlapInRadius(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != this && OtherActor->GetClass()->IsChildOf<AEnemyBase>())
	{
		EnemiesInRange.Add(OtherActor);
	}
	else if ((gameMode != 1 && gameMode != 4) && OtherActor != this && OtherActor->GetClass()->IsChildOf<AFighterBase>())
	{
		EnemiesInRange.Add(OtherActor);
	}
}

void APlayerCharacter::EndActorOverlapInRadius(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != this && OtherActor->GetClass()->IsChildOf<AFighterBase>())
	{
		EnemiesInRange.Remove(OtherActor);
	}
}

void APlayerCharacter::TogglePlayerDebug() //Prints player's states
{
	if (!playerDebug)
	{
		playerDebug = true;
	}
	else
	{
		playerDebug = false;
	}
}

void APlayerCharacter::ToggleEnemyDebug() //Prints enemy's states
{
	if (Target != nullptr)
	{
		AEnemyBase* enemyTargetDebug = Cast<AEnemyBase>(Target);
		if (enemyTargetDebug != nullptr)
		{
			if (enemyTargetDebug->enemyDebug != true)
			{
				enemyTargetDebug->SetDebug(true);
			}
			else
			{
				enemyTargetDebug->SetDebug(false);
			}
		}
	}
	
}

void APlayerCharacter::DisplayDebugInfo()
{
	if (playerDebug)
	{
		PrintStates();

		FVector backDir = -GetActorForwardVector();
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (backDir * 250.f), 10.f, FColor::Yellow, false, -1.f, (uint8)0U, 20.f);

		if (Target)
		{
			FVector targetDirection1 = Target->GetActorLocation() - GetActorLocation();
			FVector weightedRotation1 = weightedForwardInput + weightedHorizontalInput;

			weightedRotation1.Normalize();
			targetDirection1.Normalize();
			float targetYaw1 = FVector::DotProduct(weightedRotation1, targetDirection1);

			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (targetDirection1 * 250.f), 10.f, FColor::Green, false, -1.f, (uint8)0U, 20.f);
			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (weightedRotation1 * 250.f), 10.f, FColor::Red, false, -1.f, (uint8)0U, 20.f);
		}

		for (AActor* NearEnemy : EnemiesInRange) //Prints enemy yaw for debugging
		{
			if (NearEnemy)
			{
				FVector nearEnemyDirection = NearEnemy->GetActorLocation() - GetActorLocation();
				FRotator nearEnemyDifference = UKismetMathLibrary::NormalizedDeltaRotator(Controller->GetControlRotation(), nearEnemyDirection.ToOrientationRotator());
				float nearEnemyYaw = nearEnemyDifference.Yaw;
			}
		}
	}
}

