// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialBox.h"

// Sets default values
ATutorialBox::ATutorialBox()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATutorialBox::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATutorialBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

int ATutorialBox::GetBoxType()
{
	return boxType;
}

