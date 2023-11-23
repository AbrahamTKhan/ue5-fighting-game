// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialBox.generated.h"

UCLASS()
class CE301CAPSTONEPROJECT_API ATutorialBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATutorialBox();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	int GetBoxType();

private:
	UPROPERTY(EditInstanceOnly)
		int boxType = 0;

};
