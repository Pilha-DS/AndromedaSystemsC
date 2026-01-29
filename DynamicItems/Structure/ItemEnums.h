// Dynamic item system // Version 1.0.0 // date: 2026-01-29 // last update: 2026-01-29 // Author: Pilha-DS // Structure1: MasterItem

#pragma once

#include "CoreMinimal.h"
#include "ItemEnums.generated.h"

UENUM(BlueprintType)
enum class EMeshType : uint8
{
	Static		UMETA(DisplayName = "Static"),
	Skeletal	UMETA(DisplayName = "Skeletal")
};

UENUM(BlueprintType)
enum class EItemState : uint8
{
	None		UMETA(DisplayName = "None"),
	Broken		UMETA(DisplayName = "Broken"),
	Encrypted	UMETA(DisplayName = "Encrypted"),
	Anomalous	UMETA(DisplayName = "Anomalous"),
	Synthetic	UMETA(DisplayName = "Synthetic"),
	Optimized	UMETA(DisplayName = "Optimized"),
	Evolved		UMETA(DisplayName = "Evolved"),
	Absolute	UMETA(DisplayName = "Absolute"),
	Void		UMETA(DisplayName = "Void")
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	None		UMETA(DisplayName = "None"),
	Prototype	UMETA(DisplayName = "Prototype"),		// Amarelo
	Unstable	UMETA(DisplayName = "Unstable"),		// Verde
	Stable		UMETA(DisplayName = "Stable"),		// Azul
	Enhanced	UMETA(DisplayName = "Enhanced"),		// Branco
	Quantum		UMETA(DisplayName = "Quantum"),		// Laranja
	Singularity	UMETA(DisplayName = "Singularity")	// Vermelho
};

UENUM(BlueprintType)
enum class EDirectionRotation : uint8
{
	X	UMETA(DisplayName = "X"),
	Y	UMETA(DisplayName = "Y"),
	Z	UMETA(DisplayName = "Z")
};
