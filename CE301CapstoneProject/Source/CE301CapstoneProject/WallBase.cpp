// Fill out your copyright notice in the Description page of Project Settings.

#include "WallBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"

// Sets default values
AWallBase::AWallBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WallCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("Wall Collider"));
	WallCollider->bNavigationRelevant = true;
	WallCollider->SetCanEverAffectNavigation(true);
	RootComponent = WallCollider;
	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall Mesh"));
	WallMesh->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AWallBase::BeginPlay()
{
	Super::BeginPlay();
	UGameplayStatics::SpawnSoundAtLocation(this, WallMovingSFX, GetActorLocation(),FRotator::ZeroRotator, 1, 1);
	startLocation = GetActorLocation();
}

// Called every frame
void AWallBase::Tick(float DeltaTime) //Moves in to position then stops
{
	Super::Tick(DeltaTime);
	if (GetActorLocation().Z - startLocation.Z < 400.f)
	{
		FVector newLocation = GetActorLocation();
		newLocation.Z += speed * DeltaTime;
		SetActorRelativeLocation(newLocation);
	}
	else
	{
		SetActorTickEnabled(false);
	}
}

