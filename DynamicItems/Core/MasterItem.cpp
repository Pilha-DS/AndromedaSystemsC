// Version: 1.0.0
// Author: Pilha-DS
// Creation Date: 2026-01-27 // Last Update: 2026-01-27
// Description: Structures for the Dynamic Items system
// Company: Pilha-DS // copyright (c) 2026 Pilha-DS. All Rights Reserved.

#include "MasterItem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"
#include "Andromeda.h"

AMasterItem::AMasterItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = StaticMeshComponent;
	StaticMeshComponent->SetVisibility(true);
	StaticMeshComponent->SetHiddenInGame(false);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(RootComponent);
	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->SetSphereRadius(150.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	DirectionalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DirectionalLight"));
	DirectionalLight->SetupAttachment(RootComponent);
	DirectionalLight->SetVisibility(false);
	DirectionalLight->SetIntensity(0.0f);

	DebugMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugMeshComponent"));
	DebugMeshComponent->SetupAttachment(RootComponent);
	DebugMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DebugMeshComponent->SetVisibility(true);
	DebugMeshComponent->SetHiddenInGame(true);
	DebugMeshComponent->SetCastShadow(false);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultDebugMesh(TEXT("/Game/LevelPrototyping/Interactable/Target/Assets/SM_TargetBaseMesh"));
	if (DefaultDebugMesh.Succeeded())
	{
		DebugMeshComponent->SetStaticMesh(DefaultDebugMesh.Object);
		EditorDebugMesh = DefaultDebugMesh.Object;
	}
}

void AMasterItem::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ValidateItemProperties();

	if (Name.IsEmpty())
	{
		Destroy();
		return;
	}

	SetupMesh();
	SetupPhysics();
	SetupCollision();
	SetupLight();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMasterItem::OnOverlapBegin);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &AMasterItem::OnOverlapEnd);
}

void AMasterItem::BeginPlay()
{
	Super::BeginPlay();

	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		InitialMeshLocation = MeshComp->GetComponentLocation();
		InitialMeshRotation = MeshComp->GetComponentRotation();
		CurrentFloatingLocation = InitialMeshLocation;
		TargetFloatingLocation = InitialMeshLocation;
		CurrentRotation = InitialMeshRotation;
		
		if (!FloatingSettings.AlwaysFloating)
		{
			MeshComp->SetWorldLocation(InitialMeshLocation);
		}
	}

	if (FloatingSettings.AlwaysFloating)
	{
		bIsFloating = true;
	}

	if (RotationSettings.AlwaysRotate && FloatingSettings.AlwaysFloating)
	{
		if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
		{
			CurrentRotation = MeshComp->GetComponentRotation();
			bIsResettingRotation = false;
		}
		else
		{
			CurrentRotation = FRotator::ZeroRotator;
		}
	}

	UpdatePhysics();

	if (LightSettings.AlwaysLight && FloatingSettings.AlwaysFloating)
	{
		UpdateLight();
	}

	if (CollisionSphereSettings.ShowOverlappingArea)
	{
		UpdateDebugMeshVisibility();
	}

	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		if (IsValid(MeshComp))
		{
			MeshComp->OnComponentHit.AddDynamic(this, &AMasterItem::OnHit);
		}
	}
}

void AMasterItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateFloating(DeltaTime);
	UpdateRotation(DeltaTime);

	if (ActivePlayer != nullptr)
	{
		bool bIsEPressed = false;
		
		if (ACharacter* PlayerCharacter = Cast<ACharacter>(ActivePlayer))
		{
			if (APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController()))
			{
				if (PlayerController)
				{
					bIsEPressed = PlayerController->IsInputKeyDown(EKeys::E);
				}
			}
		}
		else if (APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer))
		{
			bIsEPressed = PlayerController->IsInputKeyDown(EKeys::E);
		}

		if (bIsEPressed && !bWasEPressedLastFrame)
		{
			ProcessItemPickup(ActivePlayer);
		}

		bWasEPressedLastFrame = bIsEPressed;
	}
	else
	{
		bWasEPressedLastFrame = false;
	}

	if (CollisionSphereSettings.ShowOverlappingArea && CollisionSphere)
	{
		FVector SphereLocation = CollisionSphere->GetComponentLocation();
		float SphereRadius = CollisionSphere->GetScaledSphereRadius();
		
		DrawDebugSphere(
			GetWorld(),
			SphereLocation,
			SphereRadius,
			32,
			FColor::Red,
			false,
			-1.0f,
			0,
			2.0f
		);
	}
}

void AMasterItem::ValidateItemProperties()
{
	if (!STQty.Stackable)
	{
		Quantity = 1;
	}
	else
	{
		if (Quantity <= 0)
		{
			Quantity = 1;
		}
		else if (Quantity > STQty.MaxQty)
		{
			Quantity = STQty.MaxQty;
		}
	}
}

void AMasterItem::SetupMesh()
{
	if (STModel.MeshType == EItemMeshType::Static)
	{
		bool bHasMesh = STModel.StaticMesh.ToSoftObjectPath().IsValid();
		if (bHasMesh)
		{
			UStaticMesh* LoadedMesh = STModel.StaticMesh.LoadSynchronous();
			if (LoadedMesh)
			{
				StaticMeshComponent->SetStaticMesh(LoadedMesh);
			}
			else
			{
				bHasMesh = false;
			}
		}
		StaticMeshComponent->SetVisibility(bHasMesh);
		StaticMeshComponent->SetHiddenInGame(!bHasMesh);
		StaticMeshComponent->SetWorldScale3D(STModel.Size);
		SkeletalMeshComponent->SetVisibility(false);
		SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		RootComponent = StaticMeshComponent;
		
		if (CollisionSphere && CollisionSphere->GetAttachParent() != RootComponent)
		{
			CollisionSphere->SetupAttachment(RootComponent);
		}
		if (DirectionalLight && DirectionalLight->GetAttachParent() != RootComponent)
		{
			DirectionalLight->SetupAttachment(RootComponent);
		}
		if (DebugMeshComponent && DebugMeshComponent->GetAttachParent() != RootComponent)
		{
			DebugMeshComponent->SetupAttachment(RootComponent);
		}

		if (DebugMeshComponent)
		{
			if (bHasMesh || CollisionSphereSettings.ShowOverlappingArea)
			{
				DebugMeshComponent->SetVisibility(false);
				DebugMeshComponent->SetHiddenInGame(true);
				DebugMeshComponent->SetMaterial(0, nullptr);
			}
			else
			{
				if (EditorDebugMesh.ToSoftObjectPath().IsValid())
				{
					UStaticMesh* LoadedDebugMesh = EditorDebugMesh.LoadSynchronous();
					if (LoadedDebugMesh)
					{
						DebugMeshComponent->SetStaticMesh(LoadedDebugMesh);
					}
					else
					{
						static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultDebugMesh(TEXT("/Game/LevelPrototyping/Interactable/Target/Assets/SM_TargetBaseMesh"));
						if (DefaultDebugMesh.Succeeded())
						{
							DebugMeshComponent->SetStaticMesh(DefaultDebugMesh.Object);
						}
					}
				}
				else
				{
					static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultDebugMesh(TEXT("/Game/LevelPrototyping/Interactable/Target/Assets/SM_TargetBaseMesh"));
					if (DefaultDebugMesh.Succeeded())
					{
						DebugMeshComponent->SetStaticMesh(DefaultDebugMesh.Object);
					}
				}
				
				DebugMeshComponent->SetVisibility(true);
				DebugMeshComponent->SetHiddenInGame(false);
			}
		}
		
		if (!bHasMesh)
		{
			StaticMeshComponent->SetVisibility(false);
		}
	}
	else
	{
		bool bHasMesh = STModel.SkeletalMesh.ToSoftObjectPath().IsValid();
		if (bHasMesh)
		{
			USkeletalMesh* LoadedMesh = STModel.SkeletalMesh.LoadSynchronous();
			if (LoadedMesh)
			{
				SkeletalMeshComponent->SetSkeletalMesh(LoadedMesh);
			}
			else
			{
				bHasMesh = false;
			}
		}
		SkeletalMeshComponent->SetVisibility(bHasMesh);
		SkeletalMeshComponent->SetHiddenInGame(!bHasMesh);
		SkeletalMeshComponent->SetWorldScale3D(STModel.Size);
		StaticMeshComponent->SetVisibility(false);
		StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		RootComponent = SkeletalMeshComponent;
		StaticMeshComponent->SetupAttachment(RootComponent);
		
		if (CollisionSphere && CollisionSphere->GetAttachParent() != RootComponent)
		{
			CollisionSphere->SetupAttachment(RootComponent);
		}
		if (DirectionalLight && DirectionalLight->GetAttachParent() != RootComponent)
		{
			DirectionalLight->SetupAttachment(RootComponent);
		}
		if (DebugMeshComponent && DebugMeshComponent->GetAttachParent() != RootComponent)
		{
			DebugMeshComponent->SetupAttachment(RootComponent);
		}

		if (DebugMeshComponent)
		{
			if (bHasMesh || CollisionSphereSettings.ShowOverlappingArea)
			{
				DebugMeshComponent->SetVisibility(false);
				DebugMeshComponent->SetHiddenInGame(true);
				DebugMeshComponent->SetMaterial(0, nullptr);
			}
			else
			{
				if (EditorDebugMesh.ToSoftObjectPath().IsValid())
				{
					UStaticMesh* LoadedDebugMesh = EditorDebugMesh.LoadSynchronous();
					if (LoadedDebugMesh)
					{
						DebugMeshComponent->SetStaticMesh(LoadedDebugMesh);
					}
					else
					{
						static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultDebugMesh(TEXT("/Game/LevelPrototyping/Interactable/Target/Assets/SM_TargetBaseMesh"));
						if (DefaultDebugMesh.Succeeded())
						{
							DebugMeshComponent->SetStaticMesh(DefaultDebugMesh.Object);
						}
					}
				}
				else
				{
					static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultDebugMesh(TEXT("/Game/LevelPrototyping/Interactable/Target/Assets/SM_TargetBaseMesh"));
					if (DefaultDebugMesh.Succeeded())
					{
						DebugMeshComponent->SetStaticMesh(DefaultDebugMesh.Object);
					}
				}
				
				DebugMeshComponent->SetVisibility(true);
				DebugMeshComponent->SetHiddenInGame(false);
			}
		}
		
		if (!bHasMesh)
		{
			SkeletalMeshComponent->SetVisibility(false);
		}
	}

	SetupCollisionSphere();
}

void AMasterItem::SetupPhysics()
{
	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		MeshComp->SetSimulatePhysics(true);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void AMasterItem::SetupCollision()
{
	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	}
}

void AMasterItem::SetupCollisionSphere()
{
	if (!CollisionSphere)
	{
		return;
	}

	float MeshRadius = 0.0f;
	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		FBoxSphereBounds Bounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
		FVector BoxExtent = Bounds.BoxExtent;
		MeshRadius = FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
	}

	float SphereRadius = CollisionSphereSettings.MinimumSize;
	
	if (MeshRadius > CollisionSphereSettings.MinimumSize)
	{
		SphereRadius = MeshRadius * 2.0f;
	}

	CollisionSphere->SetSphereRadius(SphereRadius);

	if (CollisionSphereSettings.ShowOverlappingArea)
	{
		CollisionSphere->SetHiddenInGame(false);
		UpdateDebugMeshVisibility();
	}
	else
	{
		CollisionSphere->SetHiddenInGame(true);
		
		if (DebugMeshComponent)
		{
			DebugMeshComponent->SetVisibility(false);
			DebugMeshComponent->SetHiddenInGame(true);
			DebugMeshComponent->SetMaterial(0, nullptr);
		}
	}
}

void AMasterItem::SetupLight()
{
	if (DirectionalLight)
	{
		DirectionalLight->SetIntensity(LightSettings.Intensity);
		DirectionalLight->SetAttenuationRadius(LightSettings.AttenuationRadius);
		DirectionalLight->SetVisibility(false);
		
		if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
		{
			FBoxSphereBounds MeshBounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
			FVector MeshSize = MeshBounds.BoxExtent;
			
			float MaxExtent = FMath::Max3(MeshSize.X, MeshSize.Y, MeshSize.Z);
			float LightOffset = FMath::Max(MaxExtent * 0.1f, 10.0f);
			
			FVector LightOffsetPosition = FVector(0.0f, 0.0f, -MeshSize.Z - LightOffset);
			DirectionalLight->SetRelativeLocation(LightOffsetPosition);
		}
	}
}

void AMasterItem::UpdatePhysics()
{
	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		bool bShouldDisablePhysics = bIsFloating || FloatingSettings.AlwaysFloating || (RotationSettings.AlwaysRotate && FloatingSettings.AlwaysFloating);
		
		if (bShouldDisablePhysics)
		{
			MeshComp->SetSimulatePhysics(false);
		}
		else
		{
			MeshComp->SetSimulatePhysics(true);
		}
	}
}

void AMasterItem::UpdateFloating(float DeltaTime)
{
	if (!FloatingSettings.Floating && !FloatingSettings.AlwaysFloating)
	{
		if (bIsFloating)
		{
			bIsFloating = false;
			UpdatePhysics();
		}
		return;
	}

	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		bool bShouldFloat = FloatingSettings.AlwaysFloating || (ActivePlayer != nullptr && FloatingSettings.Floating);

		if (bShouldFloat && !bIsFloating)
		{
			bIsFloating = true;
			
			FVector BaseLocation;
			if (bHasTouchedSurface)
			{
				BaseLocation = SurfaceContactPoint;
				InitialMeshLocation = SurfaceContactPoint;
			}
			else
			{
				BaseLocation = MeshComp->GetComponentLocation();
				InitialMeshLocation = BaseLocation;
			}
			
			CurrentFloatingLocation = MeshComp->GetComponentLocation();
			TargetFloatingLocation = BaseLocation + FVector(0.0f, 0.0f, FloatingSettings.Height);
			UpdatePhysics();
		}
		else if (!bShouldFloat && bIsFloating)
		{
			bIsFloating = false;
			UpdatePhysics();
		}

		if (bIsFloating)
		{
			FVector BaseLocation = bHasTouchedSurface ? SurfaceContactPoint : InitialMeshLocation;
			TargetFloatingLocation = BaseLocation + FVector(0.0f, 0.0f, FloatingSettings.Height);
			
			CurrentFloatingLocation = FMath::VInterpTo(
				CurrentFloatingLocation,
				TargetFloatingLocation,
				DeltaTime,
				FloatingSettings.FloatingTransitionSpeed
			);

			MeshComp->SetWorldLocation(CurrentFloatingLocation);
		}
	}
}

void AMasterItem::UpdateRotation(float DeltaTime)
{
	if (!RotationSettings.Rotate && !(RotationSettings.AlwaysRotate && FloatingSettings.AlwaysFloating))
	{
		return;
	}

	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		bool bShouldRotate = (RotationSettings.AlwaysRotate && FloatingSettings.AlwaysFloating) || 
			(ActivePlayer != nullptr && RotationSettings.Rotate && (FloatingSettings.Floating || FloatingSettings.AlwaysFloating || bIsFloating));

		if (bShouldRotate)
		{
			if (MeshComp->IsSimulatingPhysics())
			{
				MeshComp->SetSimulatePhysics(false);
			}

			if (RotationSettings.Reset && !bIsResettingRotation && !(RotationSettings.AlwaysRotate && FloatingSettings.AlwaysFloating) && !bHasResetRotation)
			{
				FRotator CurrentMeshRotation = MeshComp->GetComponentRotation();
				if (!CurrentMeshRotation.IsNearlyZero(5.0f))
				{
					bIsResettingRotation = true;
					bHasResetRotation = true;
					CurrentRotation = CurrentMeshRotation;
				}
				else
				{
					bHasResetRotation = true;
					CurrentRotation = FRotator::ZeroRotator;
					MeshComp->SetWorldRotation(FRotator::ZeroRotator);
				}
			}

			if (bIsResettingRotation)
			{
				CurrentRotation = FMath::RInterpTo(
					CurrentRotation,
					FRotator::ZeroRotator,
					DeltaTime,
					RotationSettings.ResetSpeed
				);

				MeshComp->SetWorldRotation(CurrentRotation);

				if (CurrentRotation.IsNearlyZero(1.0f))
				{
					bIsResettingRotation = false;
					CurrentRotation = FRotator::ZeroRotator;
					MeshComp->SetWorldRotation(FRotator::ZeroRotator);
				}
			}
			else
			{
				FRotator RotationDelta = FRotator::ZeroRotator;

				switch (RotationSettings.DirectionRotation)
				{
				case ERotationDirection::X:
					RotationDelta.Roll = RotationSettings.RotationSpeed * DeltaTime;
					break;
				case ERotationDirection::Y:
					RotationDelta.Yaw = RotationSettings.RotationSpeed * DeltaTime;
					break;
				case ERotationDirection::Z:
					RotationDelta.Pitch = RotationSettings.RotationSpeed * DeltaTime;
					break;
				}

				MeshComp->AddLocalRotation(RotationDelta);
				CurrentRotation = MeshComp->GetComponentRotation();
			}
		}
		else if (!(RotationSettings.AlwaysRotate && FloatingSettings.AlwaysFloating))
		{
			bIsResettingRotation = false;
			bHasResetRotation = false;
			
			if (MeshComp->IsSimulatingPhysics())
			{
				MeshComp->SetEnableGravity(true);
			}
		}
	}
}

void AMasterItem::UpdateLight()
{
	if (!LightSettings.Light && !LightSettings.AlwaysLight)
	{
		if (DirectionalLight)
		{
			DirectionalLight->SetVisibility(false);
		}
		return;
	}

	bool bShouldLight = (LightSettings.AlwaysLight && FloatingSettings.AlwaysFloating) || 
	                    (ActivePlayer != nullptr && (LightSettings.Light || LightSettings.AlwaysLight));

	if (DirectionalLight)
	{
		if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
		{
			FBoxSphereBounds MeshBounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
			FVector MeshSize = MeshBounds.BoxExtent;
			
			float MaxExtent = FMath::Max3(MeshSize.X, MeshSize.Y, MeshSize.Z);
			float LightOffset = FMath::Max(MaxExtent * 0.1f, 10.0f);
			
			FVector LightOffsetPosition = FVector(0.0f, 0.0f, -MeshSize.Z - LightOffset);
			DirectionalLight->SetRelativeLocation(LightOffsetPosition);
		}
		
		if (bShouldLight)
		{
			DirectionalLight->SetVisibility(true);
			DirectionalLight->SetIntensity(LightSettings.Intensity);
			DirectionalLight->SetAttenuationRadius(LightSettings.AttenuationRadius);

			FLinearColor RarityColor = GetRarityColor();
			DirectionalLight->SetLightColor(RarityColor);
		}
		else
		{
			DirectionalLight->SetVisibility(false);
		}
	}
}

void AMasterItem::UpdateDebugMeshVisibility()
{
	if (!DebugMeshComponent || !CollisionSphereSettings.ShowOverlappingArea)
	{
		return;
	}

	if (ActivePlayer != nullptr)
	{
		DebugMeshComponent->SetVisibility(true);
		DebugMeshComponent->SetHiddenInGame(false);
		
		static UMaterial* GreenMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EditorMaterials/WidgetMaterial_G"));
		if (GreenMaterial)
		{
			DebugMeshComponent->SetMaterial(0, GreenMaterial);
		}
	}
	else
	{
		DebugMeshComponent->SetVisibility(false);
		DebugMeshComponent->SetHiddenInGame(true);
		DebugMeshComponent->SetMaterial(0, nullptr);
	}
}

UPrimitiveComponent* AMasterItem::GetActiveMeshComponent() const
{
	if (STModel.MeshType == EItemMeshType::Static)
	{
		return StaticMeshComponent;
	}
	else
	{
		return SkeletalMeshComponent;
	}
}

FLinearColor AMasterItem::GetRarityColor() const
{
	switch (STInfos.Rarity)
	{
	case EItemRarity::None:
		return FLinearColor::White;
	case EItemRarity::Prototype:
		return FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);
	case EItemRarity::Unstable:
		return FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);
	case EItemRarity::Stable:
		return FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);
	case EItemRarity::Enhanced:
		return FLinearColor::White;
	case EItemRarity::Quantum:
		return FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);
	case EItemRarity::Singularity:
		return FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
	default:
		return FLinearColor::White;
	}
}

bool AMasterItem::IsPlayerOrCamera(AActor* Actor) const
{
	if (!IsValid(Actor) || Actor->IsTemplate())
	{
		return false;
	}

	if (Cast<ACharacter>(Actor))
	{
		return true;
	}

	if (Actor->GetComponentByClass<UCameraComponent>())
	{
		return true;
	}

	return false;
}

void AMasterItem::ProcessNextInQueue()
{
	while (WaitingQueue.Num() > 0 && ActivePlayer == nullptr)
	{
		AActor* NextPlayer = WaitingQueue[0];
		WaitingQueue.RemoveAt(0);

		if (!NextPlayer || !IsValid(NextPlayer))
		{
			continue;
		}

		if (!PlayersInRange.Contains(NextPlayer))
		{
			continue;
		}

		if (IsPlayerInCooldown(NextPlayer))
		{
			continue;
		}

		ActivePlayer = NextPlayer;
		UpdateLight();
		UpdateDebugMeshVisibility();
		
		return;
	}
}

void AMasterItem::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsPlayerOrCamera(OtherActor))
	{
		if (IsPlayerInCooldown(OtherActor))
		{
			return;
		}

		if (GetWorld())
		{
			FTimerHandle* TimerHandle = PlayerCooldownTimers.Find(OtherActor);
			if (TimerHandle && TimerHandle->IsValid())
			{
				GetWorld()->GetTimerManager().ClearTimer(*TimerHandle);
				PlayerCooldownTimers.Remove(OtherActor);
			}
		}

		PlayersInRange.Add(OtherActor);

		if (ActivePlayer != nullptr && ActivePlayer != OtherActor)
		{
			if (!WaitingQueue.Contains(OtherActor))
			{
				WaitingQueue.Add(OtherActor);
			}
			return;
		}

		if (ActivePlayer == nullptr)
		{
			WaitingQueue.Remove(OtherActor);
			ActivePlayer = OtherActor;
			UpdateLight();
			UpdateDebugMeshVisibility();
		}
	}
}

void AMasterItem::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsPlayerOrCamera(OtherActor))
	{
		WaitingQueue.Remove(OtherActor);

		if (ActivePlayer != nullptr && ActivePlayer != OtherActor)
		{
			PlayersInRange.Remove(OtherActor);
			return;
		}

		if (!PlayersInRange.Contains(OtherActor))
		{
			return;
		}

		bool bWasActivePlayer = (ActivePlayer == OtherActor);
		PlayersInRange.Remove(OtherActor);

		if (bWasActivePlayer)
		{
			ActivePlayer = nullptr;
			ProcessNextInQueue();
		}

		UpdateLight();
		UpdateDebugMeshVisibility();

		if (bWasActivePlayer && GetWorld())
		{
			FTimerHandle* ExistingTimer = PlayerCooldownTimers.Find(OtherActor);
			if (ExistingTimer && ExistingTimer->IsValid())
			{
				GetWorld()->GetTimerManager().ClearTimer(*ExistingTimer);
			}

			FTimerHandle NewTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(
				NewTimerHandle,
				FTimerDelegate::CreateUObject(this, &AMasterItem::OnOverlapCooldownFinished, OtherActor),
				5.0f,
				false
			);
			PlayerCooldownTimers.Add(OtherActor, NewTimerHandle);
		}
	}
}

bool AMasterItem::IsPlayerInCooldown(AActor* PlayerActor) const
{
	if (!PlayerActor || !GetWorld())
	{
		return false;
	}

	const FTimerHandle* TimerHandle = PlayerCooldownTimers.Find(PlayerActor);
	if (TimerHandle && TimerHandle->IsValid())
	{
		return true;
	}

	return false;
}

void AMasterItem::OnOverlapCooldownFinished(AActor* PlayerActor)
{
	if (PlayerActor && GetWorld())
	{
		FTimerHandle* TimerHandle = PlayerCooldownTimers.Find(PlayerActor);
		if (TimerHandle)
		{
			PlayerCooldownTimers.Remove(PlayerActor);
		}
	}
}

void AMasterItem::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HitComp || !IsValid(this))
	{
		return;
	}

	if (!IsValid(OtherActor) || OtherActor->IsTemplate())
	{
		return;
	}

	if (IsPlayerOrCamera(OtherActor))
	{
		return;
	}

	if (UPrimitiveComponent* MeshComp = GetActiveMeshComponent())
	{
		if (!IsValid(MeshComp))
		{
			return;
		}

		if (!bHasTouchedSurface)
		{
			SurfaceContactPoint = MeshComp->GetComponentLocation();
			bHasTouchedSurface = true;
			InitialMeshLocation = SurfaceContactPoint;
		}
	}
}

void AMasterItem::ProcessItemPickup(AActor* PlayerActor)
{
	if (!PlayerActor || !IsValid(PlayerActor))
	{
		return;
	}

	if (ActivePlayer != PlayerActor)
	{
		return;
	}

	APlayerController* PlayerController = nullptr;
	if (ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerActor))
	{
		PlayerController = Cast<APlayerController>(PlayerCharacter->GetController());
	}
	else if (APlayerController* PC = Cast<APlayerController>(PlayerActor))
	{
		PlayerController = PC;
	}

	if (PlayerController && GEngine)
	{
		FString PickupMessage = FString::Printf(TEXT("pegou %s %dx"), *Name, Quantity);
		
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Green,
			PickupMessage
		);
	}
}

void AMasterItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMasterItem, Name);
	DOREPLIFETIME(AMasterItem, ID);
	DOREPLIFETIME(AMasterItem, Quantity);
}
