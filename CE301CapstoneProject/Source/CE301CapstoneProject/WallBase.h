// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WallBase.generated.h"

UCLASS()
class CE301CAPSTONEPROJECT_API AWallBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWallBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly)
		UStaticMeshComponent* WallMesh;
	UPROPERTY(EditDefaultsOnly)
		class UBoxComponent* WallCollider;

	UPROPERTY(EditDefaultsOnly)
		class USoundBase* WallMovingSFX;
	class UAudioComponent* movingWallSound;

	UPROPERTY(EditAnywhere)
		float speed = 1500.f;

	FVector startLocation;
};
