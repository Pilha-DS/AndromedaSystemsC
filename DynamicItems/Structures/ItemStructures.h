// Version: 1.0.0
// Author: Pilha-DS
// Creation Date: 2026-01-27 // Last Update: 2026-01-27
// Description: Structures for the Dynamic Items system
// Company: Pilha-DS // copyright (c) 2026 Pilha-DS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "ItemStructures.generated.h"

UENUM(BlueprintType)
enum class EItemMeshType : uint8
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
	Prototype	UMETA(DisplayName = "Prototype"),
	Unstable	UMETA(DisplayName = "Unstable"),
	Stable		UMETA(DisplayName = "Stable"),
	Enhanced	UMETA(DisplayName = "Enhanced"),
	Quantum		UMETA(DisplayName = "Quantum"),
	Singularity	UMETA(DisplayName = "Singularity")
};

UENUM(BlueprintType)
enum class ERotationDirection : uint8
{
	X	UMETA(DisplayName = "X"),
	Y	UMETA(DisplayName = "Y"),
	Z	UMETA(DisplayName = "Z")
};

USTRUCT(BlueprintType)
struct FSTModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model")
	EItemMeshType MeshType = EItemMeshType::Static;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model", meta = (EditCondition = "MeshType == EItemMeshType::Skeletal"))
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model", meta = (EditCondition = "MeshType == EItemMeshType::Static"))
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model")
	FVector Size = FVector(1.0f, 1.0f, 1.0f);
};

USTRUCT(BlueprintType)
struct FSTQty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quantity")
	bool Stackable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quantity")
	int32 MaxQty = 20;
};

USTRUCT(BlueprintType)
struct FSTInfos
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	int32 Weight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	EItemState State = EItemState::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	EItemRarity Rarity = EItemRarity::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info", meta = (MultiLine = true))
	FString Description;
};

USTRUCT(BlueprintType)
struct FFloatingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating")
	bool Floating = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating")
	bool AlwaysFloating = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating")
	float Height = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating")
	float FloatingTransitionSpeed = 20.0f;
};

USTRUCT(BlueprintType)
struct FRotationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	bool Rotate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	bool Reset = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	bool AlwaysRotate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	float RotationSpeed = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	float ResetSpeed = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	ERotationDirection DirectionRotation = ERotationDirection::Y;
};

USTRUCT(BlueprintType)
struct FLightSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	bool Light = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	bool AlwaysLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	float Intensity = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	float AttenuationRadius = 125.0f;
};

USTRUCT(BlueprintType)
struct FCollisionSphereSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CollisionSphere")
	bool ShowOverlappingArea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CollisionSphere")
	float MinimumSize = 250.0f;
};
