// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "IconMover.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CE301CAPSTONEPROJECT_API UIconMover : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UIconMover();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Rotate(float Delta);
	void Move(float Delta);

	FRotator newRotation;
	FVector startLocation;
	float distanceFromStart;
	float newYaw;
	int forward;
	UPROPERTY(EditAnywhere)
		float rotationSpeed = 100;
	UPROPERTY(EditAnywhere)
		float moveSpeed = 10;
	UPROPERTY(EditAnywhere)
		float moveDistance = 25;
};
