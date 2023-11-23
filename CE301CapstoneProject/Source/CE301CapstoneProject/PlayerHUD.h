// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlayerHUD.generated.h"

/**
 * 
 */
UCLASS()
class CE301CAPSTONEPROJECT_API APlayerHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override; //Called when projectile is spawned

public:
	UPROPERTY()
		UUserWidget* PlayerHUD;
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UUserWidget> PlayerHUDClass;
};
