// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

UCLASS()
class CE301CAPSTONEPROJECT_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemySpawner();

	void SpawnEnemy(int newType);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
		TArray<TSubclassOf<class AEnemyBase>> enemyTypes;

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditInstanceOnly)
		int type = 1;

private:
	class UCapstoneGameInstance* gameInstance;

	int gameMode = 0;
};
