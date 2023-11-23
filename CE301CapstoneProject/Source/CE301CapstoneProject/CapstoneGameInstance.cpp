// Fill out your copyright notice in the Description page of Project Settings.


#include "CapstoneGameInstance.h"

void UCapstoneGameInstance::SetNumEnemies(int n)
{
	numEnemies = n;
}

int UCapstoneGameInstance::GetNumEnemies()
{
	return numEnemies;
}

void UCapstoneGameInstance::SetDifficulty(int n)
{
	difficulty = n;
}

int UCapstoneGameInstance::GetDifficulty()
{
	return difficulty;
}

void UCapstoneGameInstance::SetGameMode(int n)
{
	gameMode = n;
}

int UCapstoneGameInstance::GetGameMode()
{
	return gameMode;
}

void UCapstoneGameInstance::SetWaves(int n)
{
	waves = n;
}

int UCapstoneGameInstance::GetWaves()
{
	return waves;
}

void UCapstoneGameInstance::SetEnemyType(int n)
{
	enemyType = n;
}

int UCapstoneGameInstance::GetEnemyType()
{
	return enemyType;
}

void UCapstoneGameInstance::SetPlayer1Mesh(int n)
{
	player1mesh = n;
}

int UCapstoneGameInstance::GetPlayer1Mesh()
{
	return player1mesh;
}

void UCapstoneGameInstance::SetPlayer2Mesh(int n)
{
	player2mesh = n;
}

int UCapstoneGameInstance::GetPlayer2Mesh()
{
	return player2mesh;
}

