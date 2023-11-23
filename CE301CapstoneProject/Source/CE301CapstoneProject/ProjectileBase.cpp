#include "ProjectileBase.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"

#include "Particles/ParticleSystem.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"

#include "Kismet/GameplayStatics.h"

#include "FighterBase.h"
#include "PlayerFighterController.h"
#include "WallBase.h"
#include "EnemyBase.h"

AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileCollider = CreateDefaultSubobject<USphereComponent>(TEXT("Projectile Collider"));
	RootComponent = ProjectileCollider;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Move")); //Sets the speed of the projectile
	ProjectileMovement->InitialSpeed = 3500;
	ProjectileMovement->MaxSpeed = maxSpeed;
}

// Called when the game starts or when spawned
void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	lastTouchActor = GetOwner();

	loopingFire = UGameplayStatics::SpawnSoundAttached(FireLoopSFX, RootComponent); 

	checkUpcomingCollisionTimer = FTimerDelegate::CreateUObject(this, &AProjectileBase::checkUpcomingCollision); 
	GetWorldTimerManager().SetTimer(checkUpcomingCollisionHandle, checkUpcomingCollisionTimer, 0.1f, true);

	ProjectileCollider->OnComponentBeginOverlap.AddDynamic(this, &AProjectileBase::OnCollision);
	ProjectileCollider->OnComponentHit.AddDynamic(this, &AProjectileBase::ActorHitDetected);
}

void AProjectileBase::Invert(AActor* deflectActor) //Moves the projectiles towards the last actor to interact with it
{
	FVector targetVelocity = lastTouchActor->GetVelocity();
	FVector targetVelocityLocation = (lastTouchActor->GetActorLocation() + (targetVelocity * 0.5f)) - deflectActor->GetActorLocation();
	FRotator Rot = FRotationMatrix::MakeFromX(targetVelocityLocation).Rotator();
	SetActorRotation(Rot);
	ProjectileMovement->SetVelocityInLocalSpace(FVector(maxSpeed, 0, 0));

	lastTouchActor = deflectActor;

	UGameplayStatics::SpawnSoundAtLocation(this, PerfectBlockSFX, GetActorLocation());
}

void AProjectileBase::checkUpcomingCollision() //Continuously performs a line trace to see if there's an AI character nearby
{
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	FHitResult hitResult;
	if (GetWorld()->LineTraceSingleByChannel(hitResult, GetActorLocation(), GetActorLocation() + (GetActorForwardVector() * 1000), ECollisionChannel::ECC_Visibility, Params))
	{
		if (AEnemyBase* enemyActor = Cast<AEnemyBase>(hitResult.GetActor()))
		{
			enemyActor->ReactToAttack(this, 0, 0); //Allows enemies to react to the projectile
		}
	}
}

//Overlap collision
void AProjectileBase::OnCollision(UPrimitiveComponent* OverlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (otherActor != lastTouchActor)
	{
		if (hitParticleEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), hitParticleEffect, GetActorLocation());
		}

		if (otherActor->IsA<AWallBase>())
		{

			if (APlayerFighterController* playerController = Cast<APlayerFighterController>(GetWorld()->GetFirstPlayerController()))
			{
				playerController->UpdateWalls();
			}
			Invert(otherActor); //Invert if hit actor is a defensive wall
			return;
		}

		for (int i = 0; i < UGameplayStatics::GetNumPlayerControllers(this); i++)
		{

			if (ACharacter* playerFighter = UGameplayStatics::GetPlayerCharacter(this, i)) //Play camera shake to player hit actors
			{
				if (damage == 50.f && FVector::Distance(playerFighter->GetActorLocation(), GetActorLocation()) < 1000.f)
				{
					UGameplayStatics::GetPlayerController(this, i)->ClientStartCameraShake(projectileShakeClass, 1.5f);
				}
				else if (damage == 25.f && FVector::Distance(playerFighter->GetActorLocation(), GetActorLocation()) < 500.f)
				{
					UGameplayStatics::GetPlayerController(this, i)->ClientStartCameraShake(projectileShakeClass, 1.f);
				}
			}
		}
		AFighterBase* hitFighter = Cast<AFighterBase>(otherActor);
		if (hitFighter && hitFighter->GetPerfectBlockStatus())
		{
			Invert(otherActor); //Invert if the hit actor is perfect blocking
			return;
		}
		AFighterBase* projectileOwner = Cast<AFighterBase>(GetOwner());
		if (hitFighter && !hitFighter->IsFighterDead()) //Applies damage to the hit actor if the projectile's owner is still valid beforehand
		{
			if (APlayerFighterController* owningController = Cast<APlayerFighterController>(projectileOwner->GetController()))
			{
				owningController->UpdateMagicAttacks();
			};

			UClass* damageTypeClass = UDamageType::StaticClass();
			UGameplayStatics::ApplyDamage(hitFighter, damage, GetInstigatorController(), this, damageTypeClass);
			
			if (projectileOwner && projectileOwner->GetFighterTarget() == hitFighter && hitFighter->IsFighterDead())
			{
				projectileOwner->SetTarget(nullptr); //Removes the owner's target if the hit actor was the target and killed 
			}
		}

		if (FireExplosionSFX)
		{
			UGameplayStatics::SpawnSoundAtLocation(this, FireExplosionSFX, GetActorLocation());
		}
		if (loopingFire)
		{
			loopingFire->Stop();
		}
		Destroy();
	}
}

//Block collision (environment collision)
void AProjectileBase::ActorHitDetected(UPrimitiveComponent* hitComponent, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hitResult)
{
	if (hitParticleEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), hitParticleEffect, GetActorLocation());
	}
	if (FireExplosionSFX)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, FireExplosionSFX, GetActorLocation());
	}
	if (loopingFire)
	{
		loopingFire->Stop();
	}

	Destroy();
}