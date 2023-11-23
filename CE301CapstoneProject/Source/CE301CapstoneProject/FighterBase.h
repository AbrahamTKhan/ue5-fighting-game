// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FighterBase.generated.h"

UCLASS()
class CE301CAPSTONEPROJECT_API AFighterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AFighterBase(); //Constructor for fighter actors

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override; //Updates every frame

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser); //Applies damage when hit

	UFUNCTION(BlueprintPure) //Supplies health and stamina to UI blueprint
		float GetHealth();
	UFUNCTION(BlueprintPure)
		float GetStamina();

		float GetHealthAmount(); //Returns overall amount of health/stamina remaining
		float GetStaminaAmount();

	UFUNCTION(BlueprintPure)
		int GetAttackType();

	UFUNCTION(BlueprintPure)
		int GetCombatType();

	UFUNCTION(BlueprintImplementableEvent) //Character flashes white when tanking an attack
		void ShowTankFX();

	bool GetPerfectBlockStatus(); //To see if hit actor is performing a perfect block

	bool IsFighterDead();

	AActor* GetFighterTarget();

	UFUNCTION(BlueprintCallable)
		virtual void SetTarget(AActor* fighterTarget);

	void ShowLockOnWidget(bool isVisible, int playerIndex);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat") //----------------- Actor components 
		UStaticMeshComponent* MeleeWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
		class UBoxComponent* RightHandHitBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
		class UBoxComponent* RightElbowHitBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
		class UBoxComponent* LeftHandHitBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
		class UBoxComponent* RightFootHitBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
		class UBoxComponent* LeftFootHitBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = true))
		class USceneComponent* ProjectileSpawnPoint;

	UPROPERTY(EditAnywhere, Category = "Animations") //----------------- Animations
		TArray<UAnimMontage*> KickboxingLightAttacks;
	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> KickboxingHeavyAttacks;

	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> WeaponLightAttacks;
	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> WeaponHeavyAttacks;
	UPROPERTY(EditAnywhere, Category = "Animations")
		UAnimMontage* EquipAnimation;
	UPROPERTY(EditAnywhere, Category = "Animations")
		UAnimMontage* UnequipAnimation;

	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> MagicLightAttacks;
	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> MagicHeavyAttacks;

	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> SpecialAttacks;

	UPROPERTY(EditAnywhere, Category = "Animations")
		UAnimMontage* WeaponCounter;
	UPROPERTY(EditAnywhere, Category = "Animations")
		UAnimMontage* KBCounter;
	UPROPERTY(EditAnywhere, Category = "Animations")
		UAnimMontage* BlockBreakAttack;
	UPROPERTY(EditAnywhere, Category = "Animations")
		UAnimMontage* PerfectBlockAnim;
	UPROPERTY(EditAnywhere, Category = "Animations")
		UAnimMontage* BlockAnim;
	UPROPERTY(EditAnywhere, Category = "Animations")
		UAnimMontage* BlockHitReaction;
	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> HitReactions;
	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray <UAnimMontage*> RecoveryAnimations;

	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray <UAnimMontage*> EvadeAnimations;
	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray <UAnimMontage*> TauntAnimations;

	UPROPERTY(EditDefaultsOnly, Category = "Combat") //----------------- Classes to be spawned
		TSubclassOf<class AProjectileBase> LightProjectileClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
		TSubclassOf<class AProjectileBase> HeavyProjectileClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
		TSubclassOf<class AWallBase> WallBaseClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) //----------------- Screenspace UI elements
		class UWidgetComponent* LockOnWidgetP1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class UWidgetComponent* LockOnWidgetP2;

	UPROPERTY(EditDefaultsOnly) //----------------- Sound effects
		class USoundBase* AxeSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* KickboxingSFX;

	UPROPERTY(EditDefaultsOnly)
		class USoundBase* BlockSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* BlockBreakerSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* PerfectBlockSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* TankSFX;

	UPROPERTY(EditDefaultsOnly)
		class USoundBase* ForwardBackwardRollSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* SideRollSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* WhooshSFX;

	UPROPERTY(EditDefaultsOnly)
		class USoundBase* FireLightSFX;
	UPROPERTY(EditDefaultsOnly)
		class USoundBase* FireHeavySFX;

	UPROPERTY(EditAnywhere)
		class UNiagaraSystem* perfectBlockVFX;//----------------- Visual effect

	virtual void Attack();
	void LightAttack();
	void HeavyAttack();
	void SpecialAttack();
	void BlockBreaker();

	virtual void EvaluateAttackWindow(); //Called during frames that an attack does damages
	void EvaluateAttackPosition();

	void Block();
	void PerfectBlock();
	UFUNCTION(BlueprintCallable)
		virtual void StopBlocking();

	void StartRun();
	void EndRun();
	virtual void Evade();
	void Jump() override;

	void ResetAttackIndex(); //Functions that are called after a timer has expired
	void RegenerateStamina();
	void UpdatePerfectBlockStatus();
	void CanPerfectBlockCheck();
	void ClearPerfectBlockInput();

	virtual void WeaponCombatToggle();
	virtual void KickboxingCombatToggle();
	virtual void MagicCombatToggle();


	void SetUsingControllerRotation(bool condition);
	void SetAnimBlend(float newBlend);

	void Taunt();

	virtual void PrintStates();

	UFUNCTION(BlueprintCallable) //----------------- Anim notifier functions - Called at specific points during an animation
		virtual void SpawnMagicActor();
	UFUNCTION(BlueprintCallable)
		void CanAttack();
	UFUNCTION(BlueprintCallable)
		virtual void CanCounter();
	UFUNCTION(BlueprintCallable)
		virtual void CanRecover(bool can);
	UFUNCTION(BlueprintCallable) //Aligns character appropriately when starting attack
		void CanLunge(bool can);
	UFUNCTION(BlueprintCallable) //Ends attack once anim montage is complete
		virtual void StopAttacking();
	UFUNCTION(BlueprintCallable)
		virtual void SetAttackAppliesDamage(bool isDamaging); //Allows the actor to damage enemies during a specific window of the attack animation
	UFUNCTION(BlueprintCallable) //Ends hit reaction once anim montage is complete
		virtual void FinishHitReaction();
	UFUNCTION(BlueprintCallable)
		void SetIsGrounded(bool frontGrounded, bool backGrounded);
	UFUNCTION(BlueprintCallable)
		virtual void SetIsFallingOver(bool result);
	UFUNCTION(BlueprintCallable)
		void SetMovingForward(bool movementInput);
	UFUNCTION(BlueprintCallable)
		void SetMovingBackwards(bool movementInput);
	UFUNCTION(BlueprintCallable) //Anim notifier functions
		void StartEvade();
	UFUNCTION(BlueprintCallable)
		virtual void FinishEvade();
	UFUNCTION(BlueprintCallable)
		virtual void CanBlock(bool result);

	FTimerDelegate resetAttackIndex; //----------------- Timer handlers
	FTimerHandle attackIndexTimerHandle;

	FTimerDelegate regenerateStamina;
	FTimerHandle staminaTimerHandle;

	FTimerDelegate perfectBlockTimer;
	FTimerHandle perfectBlockTimerHandle;

	FTimerDelegate canPBTimer;
	FTimerHandle canPBTimerHandle;

	FTimerDelegate PBInputTimer;
	FTimerHandle PBInputTimerHandle;

	FTimerDelegate LockBlockInputTimer;
	FTimerHandle LockBlockInputTimerHandle;	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat") //----------------- Variables exposed to blueprints
		bool isTargetFocused = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
		float maxHealth = 750.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
		float maxStamina = 350.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
		float staminaRegenRate = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
		bool isEvading = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
		float animBlend = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float BaseMovementSpeed = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float FocusedMovementSpeed = 250.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float BlockingMovementSpeed = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float RunningSpeed = 600.f;
	class APlayerFighterController* playerController;//----------------- Standard variables
	class UCapstoneGameInstance* gameInstance;

	AActor* Target;

	TArray<AActor*> MeleeHitActors;

	FVector weightedInputDirection;

	TQueue<class AWallBase*> spawnedWalls; //Tracks & limits how many walls are spawned by the player
	int maxWalls = 4;
	int numWalls = 0;
	
	int gameMode = 0;

	float health = 750.f;
	float stamina = 350.f;
	float invincibilityHealth = 0.f;
	float damageRecieved = 0.f;
	
	float staminaDrained = 0.f;
	float attackDamage = 0.f;
	float appliedDamage = 0.f;
	float lungeDistance = 0.f;
	float attackIndexResetTime = 0.5f;

	float kickLightDamage = 20.f;
	float kickMediumDamage = 30.f;
	float kickSpecialDamage = 40.f;
	float weaponLightDamage = 45.f;
	float weaponMediumDamage = 60.f;
	float weaponSpecialDamage = 70.f;
	float counterDamage = 30.f;

	float runEvadeStaminaCost = 20.f;
	float airEvadeStaminaCost = 15.f;
	float standingEvadeStaminaCost = 10.f;
	float recoverStaminaCost = 40.f;

	int combatType = 1;
	int attackType = 1;
	int enemyCombatType = 1;
	int enemyAttackType = 1;

	int lightAttackIndex = 0;
	int heavyAttackIndex = 0;
	int specialAttackIndex = 0;
	int hitReactionIndex = 0;
	int evadeIndex = 0;
	int tauntIndex = 0;

	bool isAttacking = false;
	bool canCombo = false;
	bool attackLunge = false;
	bool canCounter = false;
	bool attackDealsDamage = false;
	
	bool isMovingForward = false;
	bool isMovingBackwards = false;
	bool isRunning = false;

	bool canRegenStamina = true;
	bool canRecover = false;
	
	bool isReactingToHit = false;
	bool isDead = false;

	bool isFallingOver = false;
	bool isFrontGrounded = false;
	bool isBackGrounded = false;

	bool canBlock = true;
	bool isBlocking = false;
	
	bool pbBlockInput = false;
	bool pbRunInput = false;
	bool canPerfectBlock = true;
	bool isPerfectBlocking = false;
	bool successfulPB = false;

	bool autoTurn = true;

	/*UFUNCTION(BlueprintCallable) //Returns actor's rotation in the last tick
		float GetCurrentRotationSpeed();
	float lastRotationRate = 0.f;*/
};
