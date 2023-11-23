// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CapstoneGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class CE301CAPSTONEPROJECT_API UCapstoneGameInstance : public UGameInstance
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintCallable)
		void SetNumEnemies(int n);
	
	UFUNCTION(BlueprintPure)
		int GetNumEnemies();

	UFUNCTION(BlueprintCallable)
		void SetDifficulty(int n);

	UFUNCTION(BlueprintPure)
		int GetDifficulty();

	UFUNCTION(BlueprintCallable)
		void SetGameMode(int n);

	UFUNCTION(BlueprintPure)
		int GetGameMode();

	UFUNCTION(BlueprintCallable)
		void SetWaves(int n);

	UFUNCTION(BlueprintPure)
		int GetWaves();

	UFUNCTION(BlueprintCallable)
		void SetEnemyType(int n);

	UFUNCTION(BlueprintPure)
		int GetEnemyType();

	UFUNCTION(BlueprintCallable)
		void SetPlayer1Mesh(int n);

	UFUNCTION(BlueprintPure)
		int GetPlayer1Mesh();

	UFUNCTION(BlueprintCallable)
		void SetPlayer2Mesh(int n);

	UFUNCTION(BlueprintPure)
		int GetPlayer2Mesh();
private:
	int numEnemies = 1;
	int difficulty = 2;
	int gameMode = 0;
	int waves = 1;
	int enemyType = 4;
	int player1mesh = 0;
	int player2mesh = 1;
};
