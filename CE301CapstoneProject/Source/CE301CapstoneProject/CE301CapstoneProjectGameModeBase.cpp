// Fill out your copyright notice in the Description page of Project Settings.


#include "CE301CapstoneProjectGameModeBase.h"
#include "EngineUtils.h"
#include "EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "CapstoneGameInstance.h"
#include "PlayerCharacter.h"
#include "EnemySpawner.h"
#include "GameFramework/CharacterMovementComponent.h"

void ACE301CapstoneProjectGameModeBase::BeginPlay()
{
    gameInstance = GetWorld()->GetGameInstance<UCapstoneGameInstance>();
    if (gameInstance)
    {
        gameMode = gameInstance->GetGameMode();
        numEnemies = maxEnemies = gameInstance->GetNumEnemies(); //Stores the number of enemies and players based on the game mode
        if (gameMode == 0 || gameMode == 3)
        {
            numPlayers = 1;
        }
        else
        {
            numPlayers = 2;
        }

        if (gameMode == 3 || gameMode == 4)
        {
            waves = gameInstance->GetWaves();
        }
    }

    playerIndex = 0;
    gameOver = false;
}

void ACE301CapstoneProjectGameModeBase::FighterKilled(AFighterBase* KilledFighter)
{
    if (!gameOver)
    {
        for (AFighterBase* Fighter : TActorRange<AFighterBase>(GetWorld()))
        {
            if (Fighter && KilledFighter == Fighter->GetFighterTarget())
            {
                if (!Fighter->IsFighterDead() && Fighter->IsA(AEnemyBase::StaticClass()))
                {
                    Cast<AEnemyBase>(Fighter)->GetNewTarget(false); //AI generates new target
                }
                else
                {
                    Fighter->SetTarget(nullptr); 
                }
            }
        }

        if (gameMode == 5 || gameMode == 6) //Practice modes
        {
            return;
        }
        else if (gameMode == 2) //PVP game mode
        {
            AController* fighterController = KilledFighter->GetController();
            if (fighterController && KilledFighter == UGameplayStatics::GetPlayerCharacter(this, 0)) //Checks if player 1 has died
            {
                fighterController->GameHasEnded(KilledFighter, false);
                gameOver = true;
            }
            else
            {
                fighterController->GameHasEnded(KilledFighter, true);
                gameOver = true;
            }
        }
        else //Anything else requires players to kill all enemies in order to win
        {
            if (KilledFighter->IsA(APlayerCharacter::StaticClass())) //Checks if killed fighter is a player
            {
                numPlayers--;
                if (numPlayers <= 0) //Players lose
                {
                    ACharacter* player = UGameplayStatics::GetPlayerCharacter(this, playerIndex);
                    player->GetController()->GameHasEnded(player, false);
                    gameOver = true;
                    return;
                }
                if (KilledFighter == UGameplayStatics::GetPlayerCharacter(this, 0))
                {
                    playerIndex = 1;
                }
            }
            else
            {
                numEnemies--;
                if (numEnemies <= 0 && waves > 1) //Spawn new wave
                {
                    waves--;
                    for (AEnemySpawner* Spawner : TActorRange<AEnemySpawner>(GetWorld()))
                    {
                        int randNum = FMath::RandRange(0, 3);
                        Spawner->SpawnEnemy(randNum);
                    }
                    numEnemies = maxEnemies;
                }
                else if (numEnemies <= 0) //Players win
                {
                    ACharacter* player = UGameplayStatics::GetPlayerCharacter(this, playerIndex);
                    player->GetController()->GameHasEnded(player, true);
                    gameOver = true;
                }
            }
        }
    }
}
