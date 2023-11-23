// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CE301CapstoneProjectGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class CE301CAPSTONEPROJECT_API ACE301CapstoneProjectGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

public:
	virtual void FighterKilled(class AFighterBase* KilledFighter);

private:

	class UCapstoneGameInstance* gameInstance;

	int gameMode = 0;
	int numEnemies = 0;
	int maxEnemies = 0;
	int numPlayers = 0;
	int playerIndex = 0;
	bool gameOver = 0;
	int waves = 0;
};
