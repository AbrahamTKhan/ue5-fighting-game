#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerFighterController.generated.h"

UCLASS()
class CE301CAPSTONEPROJECT_API APlayerFighterController : public APlayerController
{
	GENERATED_BODY()

public:
	void TogglePaused(bool isPaused);

	void ActivateTutorialHUD(int boxType);

	UFUNCTION(BlueprintImplementableEvent)
		void ShowTutorialHUD(const int boxType, const TSet<FString>& text);
protected:
	virtual void BeginPlay() override;

	virtual void GameHasEnded(class AActor* endTarget, bool isFighterWinner) override;
private:
	class UUserWidget* WinGameWidget;
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UUserWidget> WinGameWidgetClass;

	class UUserWidget* LoseGameWidget;
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UUserWidget> LoseGameWidgetClass;

	class UUserWidget* PauseMenuWidget;
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UUserWidget> PauseMenuWidgetClass;

	class UUserWidget* TutorialBoxWidget;
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UUserWidget> TutorialBoxWidgetClass;

	TArray<FString> textStrings;

	class UCapstoneGameInstance* gameInstance;
	int gameMode = 0;

	int lightAttacks = 0;
	int heavyAttacks = 0;
	int specialAttacks = 0;

	int kickboxingAttacks = 0;
	int weaponAttacks = 0;
	int magicAttacks = 0;

	int dodges = 0;
	int blocks = 0;
	int walls = 0;

public:
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateLightAttacks();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateHeavyAttacks();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateSpecialAttacks();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateKickboxingAttacks();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateWeaponAttacks();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateMagicAttacks();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateDodges();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateBlocks();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateWalls();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdatePerfectBlocks();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateBlockBreakers();
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateCounterAttacks();
};
