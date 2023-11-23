// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawner.h"
#include "EnemyBase.h"
#include "CapstoneGameInstance.h"

// Sets default values
AEnemySpawner::AEnemySpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	gameInstance = GetWorld()->GetGameInstance<UCapstoneGameInstance>();
	if (gameInstance)
	{
		gameMode = gameInstance->GetGameMode();
	}

	if (gameInstance && gameMode == 0 || gameMode == 1 || gameMode == 6) //Spawn selected enemy type from the menu
	{
		int setType = gameInstance->GetEnemyType();
		AEnemyBase* enemy = GetWorld()->SpawnActor<AEnemyBase>(enemyTypes[setType], GetActorTransform());
		enemy->SetOwner(this);
	}
	else //Spawn pre-determined enemy type (used in waves)
	{
		if (enemyTypes[type])
		{
			AEnemyBase* enemy = GetWorld()->SpawnActor<AEnemyBase>(enemyTypes[type], GetActorTransform());
			enemy->SetOwner(this);
		}
	}
}

void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemySpawner::SpawnEnemy(int newType = -1)
{
	if (newType < 0 && enemyTypes[type]) //Spawns pre-determined enemy type
	{
		AEnemyBase* enemy = GetWorld()->SpawnActor<AEnemyBase>(enemyTypes[type], GetActorTransform());
		enemy->SetOwner(this);
	}
	else if (enemyTypes[newType]) //Spawns type provided from the parameter
	{
		AEnemyBase* enemy = GetWorld()->SpawnActor<AEnemyBase>(enemyTypes[newType], GetActorTransform());
		enemy->SetOwner(this);
	}
}

