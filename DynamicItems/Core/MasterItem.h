// Dynamic item system // Version 1.0.0 // date: 2026-01-29 // last update: 2026-01-29 // Author: Pilha-DS // Actor: MasterItem

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/WidgetComponent.h"
#include "AndromedaSystemsC/DynamicItems/Structure/ItemStructures.h"
#include "MasterItem.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class USphereComponent;
class USpotLightComponent;
class UWidgetComponent;
class ACharacter;

/**
 * Classe base para todos os itens do jogo
 * Responsável por dados do item, visualização no mundo, interações e replicação em multiplayer
 */
UCLASS(BlueprintType, Blueprintable)
class ANDROMEDA_API AMasterItem : public AActor
{
	GENERATED_BODY()
	
public:	
	AMasterItem(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Componentes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpotLightComponent> SpotLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> WidgetInstructionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> WidgetPickupComponent;

	// Dados do Item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item")
	FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FSTModel STModel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FSTQty STQty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FSTInfos STInfos;

	// WorldView Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FBasicInfos BasicInfos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FFloatingSettings FloatingSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FRotationSettings RotationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FLightSettings LightSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FCollisionSphereSettings CollisionSphereSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldView")
	FWidgetsSettings WidgetsSettings;

	// Estados internos
	TArray<ACharacter*> OverlappingPlayers;
	TMap<ACharacter*, float> PlayerCooldowns; // Mapa de players e seus tempos de cooldown
	float OverlapCooldownTime = 5.0f; // Tempo de cooldown em segundos
	FVector OriginalLocation;
	FRotator OriginalRotation;
	FRotator CurrentRotation;
	float CurrentFloatingHeight = 0.0f;
	bool bIsFloating = false;
	bool bIsRotating = false;
	bool bIsResettingRotation = false;
	bool bIsLightOn = false;
	FVector WidgetInstructionWorldLocation; // Posição fixa do widget no mundo

	// Funções de configuração
	void SetupMesh();
	void SetupCollision();
	void SetupCollisionSphere();
	void SetupLight();
	void SetupWidgets();
	void UpdateCollisionSphereSize();

	// Funções de comportamento
	void UpdateFloating(float DeltaTime);
	void UpdateRotation(float DeltaTime);
	void UpdateLight();
	void UpdateWidgets();

	// Overlap Events
	UFUNCTION()
	void OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Validação
	void ValidateItemData();
	FLinearColor GetRarityColor() const;

	// Replicação
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Getters
	FORCEINLINE UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }
	FORCEINLINE USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }
	FORCEINLINE USphereComponent* GetCollisionSphere() const { return CollisionSphere; }
	FORCEINLINE USpotLightComponent* GetSpotLight() const { return SpotLight; }
};
