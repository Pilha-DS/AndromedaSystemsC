// Dynamic item system // Version 1.0.0 // date: 2026-01-29 // last update: 2026-01-29 // Author: Pilha-DS // Structure2: MasterItem	


#pragma once

#include "CoreMinimal.h"
#include "AndromedaSystemsC/DynamicItems/Structure/ItemEnums.h"
#include "ItemStructures.generated.h"

USTRUCT(BlueprintType)
struct FSTModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Model")
	EMeshType MeshType = EMeshType::Static;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Model", meta = (AllowedClasses = "StaticMesh"))
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Model", meta = (AllowedClasses = "SkeletalMesh"))
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Model")
	FVector Size = FVector(1.0f, 1.0f, 1.0f);
};

USTRUCT(BlueprintType)
struct FSTQty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Quantity")
	bool Stackable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Quantity")
	int32 MaxQty = 20;
};

USTRUCT(BlueprintType)
struct FSTInfos
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Infos")
	int32 Weight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Infos")
	EItemState State = EItemState::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Infos")
	EItemRarity Rarity = EItemRarity::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Infos")
	FString Description = TEXT("");
};

USTRUCT(BlueprintType)
struct FFloatingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Floating")
	bool Floating = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Floating")
	float Height = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Floating")
	float FloatingTransitionSpeed = 10.0f;
};

USTRUCT(BlueprintType)
struct FRotationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Rotation")
	bool Rotate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Rotation")
	bool Reset = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Rotation")
	float RotationSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Rotation")
	float ResetSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Rotation")
	EDirectionRotation DirectionRotation = EDirectionRotation::Y;
};

USTRUCT(BlueprintType)
struct FLightSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Light")
	bool Light = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Light")
	float Intensity = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Light")
	float AttenuationRadius = 150.0f;
};

USTRUCT(BlueprintType)
struct FCollisionSphereSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|CollisionSphere")
	bool ShowOverlappingArea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|CollisionSphere")
	float MinimumSize = 200.0f;
};

USTRUCT(BlueprintType)
struct FBasicInfos
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Basic Infos")
	bool EasyMode = false;
};

USTRUCT(BlueprintType)
struct FWidgetsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Widgets", meta = (AllowedClasses = "UserWidget"))
	TSubclassOf<UUserWidget> WidgetInstruction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Widgets")
	FVector WidgetInstructionPosition = FVector(0.0f, 150.0f, 50.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Widgets")
	FVector2D WidgetInstructionSize = FVector2D(200.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Widgets", meta = (AllowedClasses = "UserWidget"))
	TSubclassOf<UUserWidget> WidgetPickup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Widgets")
	FVector WidgetPickupPosition = FVector(0.0f, 0.0f, 150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView|Widgets")
	FVector2D WidgetPickupSize = FVector2D(200.0f, 100.0f);
};
