// Fill out your copyright notice in the Description page of Project Settings.


#include "IconMover.h"

// Sets default values for this component's properties
UIconMover::UIconMover()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UIconMover::BeginPlay()
{
	Super::BeginPlay();
	
	newRotation = GetComponentRotation();
	startLocation = GetComponentLocation();
	forward = 1;
}


// Called every frame
void UIconMover::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Move(DeltaTime);
	Rotate(DeltaTime);
}

void UIconMover::Rotate(float Delta) //Continuously spins
{
	newRotation = FRotator(0, rotationSpeed * Delta, 0);
	AddLocalRotation(newRotation);
}

void UIconMover::Move(float Delta) //Moves up and down
{
	FVector currentLoc = FVector(0, 0, (moveSpeed * Delta) * forward);

	AddLocalOffset(currentLoc);

	float zz = GetComponentLocation().Z;

	distanceFromStart = FVector::Dist(startLocation, GetComponentLocation());

	if (distanceFromStart > moveDistance)
	{
		FVector newStart = startLocation;
		newStart.Z += moveDistance * forward;
		startLocation = newStart;
		SetWorldLocation(startLocation);
		forward *= -1;
	}
}

