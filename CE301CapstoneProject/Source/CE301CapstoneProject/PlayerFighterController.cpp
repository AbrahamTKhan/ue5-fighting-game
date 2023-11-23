// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerFighterController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "CE301CapstoneProjectGameModeBase.h"
#include "CapstoneGameInstance.h"



void APlayerFighterController::BeginPlay()
{
	Super::BeginPlay();

	gameInstance = GetWorld()->GetGameInstance<UCapstoneGameInstance>();
	if (gameInstance)
	{
		gameMode = gameInstance->GetGameMode();
	}
}


void APlayerFighterController::GameHasEnded(AActor* endTarget, bool isFighterWinner)
{
	Super::GameHasEnded(endTarget, isFighterWinner);

	if (gameMode == 2)
	{
		if (isFighterWinner)
		{
			WinGameWidget = CreateWidget(this, WinGameWidgetClass); //Displays winning HUD
			if (WinGameWidget)
			{
				WinGameWidget->AddToViewport();
			}
		}
		else
		{
			LoseGameWidget = CreateWidget(this, LoseGameWidgetClass); //Displays losing HUD
			if (LoseGameWidget)
			{
				LoseGameWidget->AddToViewport();
			}
		}
	}
	else
	{
		if (isFighterWinner)
		{
			WinGameWidget = CreateWidget(this, WinGameWidgetClass); //Displays winning HUD
			if (WinGameWidget)
			{
				WinGameWidget->AddToViewport();
			}
		}
		else
		{
			LoseGameWidget = CreateWidget(this, LoseGameWidgetClass); //Displays losing HUD
			if (LoseGameWidget)
			{
				LoseGameWidget->AddToViewport();
			}
		}
	}
	
}

void APlayerFighterController::TogglePaused(bool isPaused)
{
	if (isPaused)
	{
		PauseMenuWidget = CreateWidget(this, PauseMenuWidgetClass); //Displays HUD blueprint on spawn
		if (PauseMenuWidget)
		{
			PauseMenuWidget->AddToViewport();
		}

		bShowMouseCursor = true;
		SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		PauseMenuWidget->RemoveFromViewport();

		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
	this->SetPause(isPaused);
}

void APlayerFighterController::ActivateTutorialHUD(int boxType)
{
	TSet<FString> textString;
	if (boxType == 1)
	{
		textString.Add("Well done for traversing past the gaps! Now it's time to hone your combat skills.");
	}

	ShowTutorialHUD(boxType, textString);
}