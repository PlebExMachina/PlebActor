// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlebActorsGameMode.h"
#include "PlebActorsCharacter.h"
#include "UObject/ConstructorHelpers.h"

APlebActorsGameMode::APlebActorsGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
