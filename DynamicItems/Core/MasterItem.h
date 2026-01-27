// Version: 1.0.0
// Author: Pilha-DS
// Creation Date: 2026-01-27 // Last Update: 2026-01-27
// Description: Structures for the Dynamic Items system
// Company: Pilha-DS // copyright (c) 2026 Pilha-DS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "../Structures/ItemStructures.h"
#include "MasterItem.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ANDROMEDA_API AMasterItem : public AActor
{
	GENERATED_BODY()

public:
	AMasterItem(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* DirectionalLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DebugMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", Replicated)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", Replicated)
	FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", Replicated)
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FSTModel STModel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FSTQty STQty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FSTInfos STInfos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FFloatingSettings FloatingSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FRotationSettings RotationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FLightSettings LightSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FCollisionSphereSettings CollisionSphereSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	TSoftObjectPtr<UStaticMesh> EditorDebugMesh;

	FVector InitialMeshLocation;
	FRotator InitialMeshRotation;
	TSet<AActor*> PlayersInRange;
	AActor* ActivePlayer = nullptr;
	TArray<AActor*> WaitingQueue;
	bool bWasEPressedLastFrame = false;
	bool bIsFloating = false;
	bool bIsResettingRotation = false;
	bool bHasResetRotation = false;
	FRotator CurrentRotation;
	FVector TargetFloatingLocation;
	FVector CurrentFloatingLocation;
	FVector SurfaceContactPoint;
	bool bHasTouchedSurface = false;
	TMap<AActor*, FTimerHandle> PlayerCooldownTimers;

	void ValidateItemProperties();
	void SetupMesh();
	void SetupPhysics();
	void SetupCollision();
	void SetupCollisionSphere();
	void SetupLight();
	UPrimitiveComponent* GetActiveMeshComponent() const;
	FLinearColor GetRarityColor() const;
	void UpdatePhysics();
	void UpdateFloating(float DeltaTime);
	void UpdateRotation(float DeltaTime);
	void UpdateLight();
	void UpdateDebugMeshVisibility();
	void ProcessNextInQueue();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnOverlapCooldownFinished(AActor* PlayerActor);

	bool IsPlayerInCooldown(AActor* PlayerActor) const;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool IsPlayerOrCamera(AActor* Actor) const;
	void ProcessItemPickup(AActor* PlayerActor);

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FORCEINLINE UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }
	FORCEINLINE USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }
	FORCEINLINE USphereComponent* GetCollisionSphere() const { return CollisionSphere; }
	FORCEINLINE UPointLightComponent* GetDirectionalLight() const { return DirectionalLight; }
};
