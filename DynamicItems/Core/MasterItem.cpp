// Dynamic item system // Version 1.0.0 // date: 2026-01-29 // last update: 2026-01-29 // Author: Pilha-DS // Actor: MasterItem

#include "MasterItem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"

AMasterItem::AMasterItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	// Criar StaticMeshComponent
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = StaticMeshComponent;
	StaticMeshComponent->SetSimulatePhysics(true);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	StaticMeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	// Criar SkeletalMeshComponent (inicialmente desabilitado)
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(RootComponent);
	SkeletalMeshComponent->SetSimulatePhysics(true);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SkeletalMeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SkeletalMeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	SkeletalMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	SkeletalMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetActive(false);

	// Criar CollisionSphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CollisionSphere->SetSphereRadius(50.0f);

	// Criar SpotLight
	SpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLight"));
	SpotLight->SetupAttachment(RootComponent);
	SpotLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	SpotLight->SetIntensity(LightSettings.Intensity);
	SpotLight->SetAttenuationRadius(LightSettings.AttenuationRadius);
	SpotLight->SetVisibility(false);

	// Criar WidgetComponents
	WidgetInstructionComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetInstructionComponent"));
	// WidgetInstruction não é anexado ao RootComponent, tem posição independente
	WidgetInstructionComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetInstructionComponent->SetVisibility(false);
	// Garantir que não está anexado a nenhum componente
	WidgetInstructionComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	WidgetPickupComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetPickupComponent"));
	WidgetPickupComponent->SetupAttachment(RootComponent);
	WidgetPickupComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetPickupComponent->SetVisibility(false);
}

void AMasterItem::BeginPlay()
{
	Super::BeginPlay();

	ValidateItemData();

	// Configurar componentes baseado nos dados do item
	SetupMesh();
	SetupCollision();
	SetupCollisionSphere();
	SetupLight();
	SetupWidgets();

	// Salvar posição e rotação originais
	OriginalLocation = GetActorLocation();
	OriginalRotation = GetActorRotation();
	CurrentRotation = OriginalRotation;
	
	// Salvar posição fixa do WidgetInstruction no mundo
	WidgetInstructionWorldLocation = GetActorLocation() + WidgetsSettings.WidgetInstructionPosition;

	// Configurar eventos de overlap
	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMasterItem::OnCollisionSphereBeginOverlap);
		CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &AMasterItem::OnCollisionSphereEndOverlap);
	}
}

void AMasterItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Remover players inválidos da lista (caso tenham sido destruídos)
	OverlappingPlayers.RemoveAll([](ACharacter* Player) { return !IsValid(Player); });
	
	// Limpar cooldowns expirados e players inválidos do mapa de cooldowns
	float CurrentTime = GetWorld()->GetTimeSeconds();
	TArray<ACharacter*> PlayersToRemove;
	for (auto& CooldownPair : PlayerCooldowns)
	{
		if (!IsValid(CooldownPair.Key) || (CurrentTime - CooldownPair.Value) >= OverlapCooldownTime)
		{
			PlayersToRemove.Add(CooldownPair.Key);
		}
	}
	for (ACharacter* PlayerToRemove : PlayersToRemove)
	{
		PlayerCooldowns.Remove(PlayerToRemove);
	}

	// Verificar se há pelo menos um player overlapping
	bool bHasOverlappingPlayers = OverlappingPlayers.Num() > 0;
	bool bEasyModeActive = BasicInfos.EasyMode;

	// Manter WidgetInstruction em posição fixa no mundo (sempre, independente do estado)
	if (WidgetInstructionComponent)
	{
		WidgetInstructionComponent->SetWorldLocation(WidgetInstructionWorldLocation);
	}

	// Se EasyMode está ativo ou há players overlapping, ativar efeitos
	if (bEasyModeActive || bHasOverlappingPlayers)
	{
		// Se EasyMode está ativo e não há players, garantir que estados estejam inicializados
		if (bEasyModeActive && !bHasOverlappingPlayers)
		{
			// Inicializar estados apenas uma vez quando EasyMode é ativado
			if (!bIsFloating)
			{
				OriginalLocation = GetActorLocation();
				OriginalRotation = GetActorRotation();
				CurrentRotation = OriginalRotation;
				bIsRotating = false;
				bIsResettingRotation = false;
				// Garantir que a luz seja ligada no EasyMode
				bIsLightOn = false; // Resetar para forçar ativação
			}
		}

		UpdateFloating(DeltaTime);
		UpdateRotation(DeltaTime);
		UpdateLight();
		
		// Widgets só aparecem se houver players overlapping
		UpdateWidgets();
	}
	else
	{
		// Apenas parar de rotacionar, sem resetar
		if (bIsRotating)
		{
			bIsRotating = false;
		}

		// Desativar floating e reativar física se necessário
		if (bIsFloating)
		{
			bIsFloating = false;
			// Reativar física imediatamente na posição atual
			if (StaticMeshComponent && StaticMeshComponent->IsVisible())
			{
				StaticMeshComponent->SetSimulatePhysics(true);
			}
			if (SkeletalMeshComponent && SkeletalMeshComponent->IsVisible())
			{
				SkeletalMeshComponent->SetSimulatePhysics(true);
			}
		}

		// Desligar luz
		if (bIsLightOn)
		{
			bIsLightOn = false;
			if (SpotLight)
			{
				SpotLight->SetVisibility(false);
			}
		}

		// Esconder widgets (UpdateWidgets já verifica o player local)
		UpdateWidgets();
	}
}

void AMasterItem::SetupMesh()
{
	if (STModel.MeshType == EMeshType::Static)
	{
		// Verificar se o StaticMesh está configurado
		if (!STModel.StaticMesh.IsNull())
		{
			UStaticMesh* LoadedMesh = STModel.StaticMesh.LoadSynchronous();
			if (LoadedMesh && StaticMeshComponent)
			{
				StaticMeshComponent->SetStaticMesh(LoadedMesh);
				StaticMeshComponent->SetWorldScale3D(STModel.Size);
				StaticMeshComponent->SetVisibility(true);
				StaticMeshComponent->SetActive(true);
				
				// Garantir que StaticMeshComponent seja o RootComponent
				if (RootComponent != StaticMeshComponent)
				{
					// Reattachar componentes ao novo RootComponent
					if (CollisionSphere)
					{
						CollisionSphere->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						CollisionSphere->SetupAttachment(StaticMeshComponent);
					}
					if (SpotLight)
					{
						SpotLight->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						SpotLight->SetupAttachment(StaticMeshComponent);
					}
					// WidgetInstruction não é anexado, mantém posição independente
					if (WidgetPickupComponent)
					{
						WidgetPickupComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						WidgetPickupComponent->SetupAttachment(StaticMeshComponent);
					}
					
					RootComponent = StaticMeshComponent;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AMasterItem: Falha ao carregar StaticMesh para %s"), *GetName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AMasterItem: StaticMesh não configurado para %s"), *GetName());
		}
		
		// Desabilitar SkeletalMeshComponent
		if (SkeletalMeshComponent)
		{
			SkeletalMeshComponent->SetVisibility(false);
			SkeletalMeshComponent->SetActive(false);
		}
	}
	else if (STModel.MeshType == EMeshType::Skeletal)
	{
		// Verificar se o SkeletalMesh está configurado
		if (!STModel.SkeletalMesh.IsNull())
		{
			USkeletalMesh* LoadedMesh = STModel.SkeletalMesh.LoadSynchronous();
			if (LoadedMesh && SkeletalMeshComponent)
			{
				SkeletalMeshComponent->SetSkeletalMesh(LoadedMesh);
				SkeletalMeshComponent->SetWorldScale3D(STModel.Size);
				SkeletalMeshComponent->SetVisibility(true);
				SkeletalMeshComponent->SetActive(true);
				
				// Garantir que SkeletalMeshComponent seja o RootComponent
				if (RootComponent != SkeletalMeshComponent)
				{
					// Reattachar componentes ao novo RootComponent
					if (CollisionSphere)
					{
						CollisionSphere->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						CollisionSphere->SetupAttachment(SkeletalMeshComponent);
					}
					if (SpotLight)
					{
						SpotLight->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						SpotLight->SetupAttachment(SkeletalMeshComponent);
					}
					// WidgetInstruction não é anexado, mantém posição independente
					if (WidgetPickupComponent)
					{
						WidgetPickupComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						WidgetPickupComponent->SetupAttachment(SkeletalMeshComponent);
					}
					
					RootComponent = SkeletalMeshComponent;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AMasterItem: Falha ao carregar SkeletalMesh para %s"), *GetName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AMasterItem: SkeletalMesh não configurado para %s"), *GetName());
		}
		
		// Desabilitar StaticMeshComponent
		if (StaticMeshComponent)
		{
			StaticMeshComponent->SetVisibility(false);
			StaticMeshComponent->SetActive(false);
		}
	}
}

void AMasterItem::SetupCollision()
{
	// Configuração já feita no construtor, mas podemos ajustar aqui se necessário
}

void AMasterItem::SetupCollisionSphere()
{
	if (CollisionSphere)
	{
		UpdateCollisionSphereSize();
		CollisionSphere->SetHiddenInGame(!CollisionSphereSettings.ShowOverlappingArea);
	}
}

void AMasterItem::UpdateCollisionSphereSize()
{
	if (!CollisionSphere) return;

	FVector MeshBounds = FVector::ZeroVector;
	
	if (StaticMeshComponent && StaticMeshComponent->IsVisible() && StaticMeshComponent->GetStaticMesh())
	{
		FBoxSphereBounds Bounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
		MeshBounds = Bounds.BoxExtent * 2.0f * STModel.Size;
	}
	else if (SkeletalMeshComponent && SkeletalMeshComponent->IsVisible() && SkeletalMeshComponent->GetSkeletalMeshAsset())
	{
		FBoxSphereBounds Bounds = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetBounds();
		MeshBounds = Bounds.BoxExtent * 2.0f * STModel.Size;
	}

	float MaxDimension = FMath::Max3(MeshBounds.X, MeshBounds.Y, MeshBounds.Z);
	float SphereRadius;
	
	// Se o mesh for menor que o tamanho mínimo, usar o tamanho mínimo
	// Caso contrário, usar o dobro do tamanho do mesh
	if (MaxDimension < CollisionSphereSettings.MinimumSize)
	{
		SphereRadius = CollisionSphereSettings.MinimumSize;
	}
	else
	{
		SphereRadius = MaxDimension * 2.0f;
	}
	
	CollisionSphere->SetSphereRadius(SphereRadius);
}

void AMasterItem::SetupLight()
{
	if (SpotLight)
	{
		SpotLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		SpotLight->SetIntensity(LightSettings.Intensity);
		SpotLight->SetAttenuationRadius(LightSettings.AttenuationRadius);
		SpotLight->SetLightColor(GetRarityColor());
		SpotLight->SetVisibility(false);
	}
}

void AMasterItem::SetupWidgets()
{
	if (WidgetInstructionComponent)
	{
		WidgetInstructionComponent->SetWidgetSpace(EWidgetSpace::Screen);
		WidgetInstructionComponent->SetDrawSize(WidgetsSettings.WidgetInstructionSize);
		// Garantir que não está anexado a nenhum componente
		WidgetInstructionComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		// WidgetInstruction usa posição fixa no mundo, não relativa ao item
		WidgetInstructionWorldLocation = GetActorLocation() + WidgetsSettings.WidgetInstructionPosition;
		WidgetInstructionComponent->SetWorldLocation(WidgetInstructionWorldLocation);
		if (WidgetsSettings.WidgetInstruction)
		{
			WidgetInstructionComponent->SetWidgetClass(WidgetsSettings.WidgetInstruction);
		}
		WidgetInstructionComponent->SetVisibility(false);
	}

	if (WidgetPickupComponent)
	{
		WidgetPickupComponent->SetWidgetSpace(EWidgetSpace::Screen);
		WidgetPickupComponent->SetDrawSize(WidgetsSettings.WidgetPickupSize);
		WidgetPickupComponent->SetRelativeLocation(WidgetsSettings.WidgetPickupPosition);
		if (WidgetsSettings.WidgetPickup)
		{
			WidgetPickupComponent->SetWidgetClass(WidgetsSettings.WidgetPickup);
		}
		WidgetPickupComponent->SetVisibility(false);
	}
}

void AMasterItem::UpdateFloating(float DeltaTime)
{
	if (!FloatingSettings.Floating) return;

	UStaticMeshComponent* ActiveMesh = StaticMeshComponent && StaticMeshComponent->IsVisible() ? StaticMeshComponent : nullptr;
	USkeletalMeshComponent* ActiveSkeletalMesh = SkeletalMeshComponent && SkeletalMeshComponent->IsVisible() ? SkeletalMeshComponent : nullptr;

	if (!ActiveMesh && !ActiveSkeletalMesh) return;

	// Ativar floating
	if (!bIsFloating)
	{
		bIsFloating = true;
		if (ActiveMesh)
		{
			ActiveMesh->SetSimulatePhysics(false);
		}
		if (ActiveSkeletalMesh)
		{
			ActiveSkeletalMesh->SetSimulatePhysics(false);
		}
		OriginalLocation = GetActorLocation();
	}

	// Interpolar altura
	float TargetHeight = OriginalLocation.Z + FloatingSettings.Height;
	FVector CurrentLocation = GetActorLocation();
	FVector TargetLocation = FVector(CurrentLocation.X, CurrentLocation.Y, TargetHeight);
	FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, FloatingSettings.FloatingTransitionSpeed);
	
	SetActorLocation(NewLocation);
}

void AMasterItem::UpdateRotation(float DeltaTime)
{
	if (!RotationSettings.Rotate) return;

	bool bEasyModeActive = BasicInfos.EasyMode;

	// Em EasyMode, pular o reset e começar a rotacionar diretamente
	if (bEasyModeActive)
	{
		if (!bIsRotating)
		{
			bIsRotating = true;
			bIsResettingRotation = false;
		}
	}
	else
	{
		// Se precisa resetar e ainda não terminou o reset
		if (RotationSettings.Reset && !bIsResettingRotation && !bIsRotating)
		{
			bIsResettingRotation = true;
		}

		// Se está resetando, interpolar para zero
		if (bIsResettingRotation)
		{
			FRotator CurrentRot = GetActorRotation();
			FRotator TargetRotation = FRotator::ZeroRotator;
			
			// Interpolar para a rotação zero
			FRotator NewRotation = FMath::RInterpTo(CurrentRot, TargetRotation, DeltaTime, RotationSettings.ResetSpeed);
			SetActorRotation(NewRotation);
			
			// Verificar se chegou perto de zero
			if (FMath::IsNearlyEqual(NewRotation.Yaw, 0.0f, 1.0f) &&
				FMath::IsNearlyEqual(NewRotation.Pitch, 0.0f, 1.0f) &&
				FMath::IsNearlyEqual(NewRotation.Roll, 0.0f, 1.0f))
			{
				// Reset completo, pode começar a rotacionar
				SetActorRotation(TargetRotation); // Garantir que está exatamente em zero
				bIsResettingRotation = false;
				bIsRotating = true;
			}
			else
			{
				// Ainda está resetando, não rotacionar ainda
				return;
			}
		}

		// Se não precisa resetar e ainda não começou a rotacionar, começar agora
		if (!RotationSettings.Reset && !bIsRotating)
		{
			bIsRotating = true;
			OriginalRotation = GetActorRotation();
		}
	}

	// Agora rotacionar
	if (bIsRotating)
	{
		FRotator CurrentRot = GetActorRotation();
		FRotator DeltaRotation = FRotator::ZeroRotator;

		float RotationDelta = RotationSettings.RotationSpeed * DeltaTime;

		switch (RotationSettings.DirectionRotation)
		{
		case EDirectionRotation::X:
			DeltaRotation.Roll = RotationDelta;
			break;
		case EDirectionRotation::Y:
			DeltaRotation.Yaw = RotationDelta;
			break;
		case EDirectionRotation::Z:
			DeltaRotation.Pitch = RotationDelta;
			break;
		}

		FRotator NewRotation = CurrentRot + DeltaRotation;
		SetActorRotation(NewRotation);
	}
}

void AMasterItem::UpdateLight()
{
	if (!LightSettings.Light) return;

	if (SpotLight)
	{
		if (!bIsLightOn)
		{
			bIsLightOn = true;
			SpotLight->SetVisibility(true);
			SpotLight->SetLightColor(GetRarityColor());
		}
		// Garantir que a luz permaneça acesa
		if (!SpotLight->IsVisible())
		{
			SpotLight->SetVisibility(true);
		}
	}
}

void AMasterItem::UpdateWidgets()
{
	// Verificar se o player local está na lista de overlapping players
	APlayerController* LocalPlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	ACharacter* LocalPlayerCharacter = LocalPlayerController ? Cast<ACharacter>(LocalPlayerController->GetPawn()) : nullptr;
	
	bool bLocalPlayerIsOverlapping = LocalPlayerCharacter && OverlappingPlayers.Contains(LocalPlayerCharacter);

	if (bLocalPlayerIsOverlapping)
	{
		if (WidgetInstructionComponent && WidgetsSettings.WidgetInstruction)
		{
			WidgetInstructionComponent->SetVisibility(true);
		}

		if (WidgetPickupComponent && WidgetsSettings.WidgetPickup)
		{
			WidgetPickupComponent->SetVisibility(true);
		}
	}
	else
	{
		// Esconder widgets se o player local não está overlapping
		if (WidgetInstructionComponent)
		{
			WidgetInstructionComponent->SetVisibility(false);
		}
		if (WidgetPickupComponent)
		{
			WidgetPickupComponent->SetVisibility(false);
		}
	}
}

void AMasterItem::OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		// Se já houver um player na lista, ignorar completamente este evento
		if (OverlappingPlayers.Num() > 0)
		{
			return;
		}
		
		// Verificar se o player está em cooldown
		if (PlayerCooldowns.Contains(Character))
		{
			float CurrentTime = GetWorld()->GetTimeSeconds();
			float CooldownEndTime = PlayerCooldowns[Character];
			if ((CurrentTime - CooldownEndTime) < OverlapCooldownTime)
			{
				// Player ainda está em cooldown, ignorar
				return;
			}
			else
			{
				// Cooldown expirado, remover do mapa
				PlayerCooldowns.Remove(Character);
			}
		}
		
		// Verificar se o player já está na lista (não deveria estar, mas verificação de segurança)
		if (!OverlappingPlayers.Contains(Character))
		{
			// Adicionar à lista (garantido que está vazia)
			OverlappingPlayers.Add(Character);
			
			// Inicializar estados para o primeiro player
			OriginalLocation = GetActorLocation();
			OriginalRotation = GetActorRotation();
			CurrentRotation = OriginalRotation;
			// Resetar flags de rotação para que reset aconteça antes de começar a rotacionar
			bIsRotating = false;
			bIsResettingRotation = false;
		}
	}
}

void AMasterItem::OnCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		// Só processar se o player estiver na lista (apenas o player autorizado)
		if (OverlappingPlayers.Contains(Character))
		{
			// Remover da lista (só pode haver um player por vez)
			OverlappingPlayers.Remove(Character);
			
			// Registrar o tempo de saída para iniciar o cooldown
			float CurrentTime = GetWorld()->GetTimeSeconds();
			PlayerCooldowns.Add(Character, CurrentTime);
			
			// Os efeitos serão desativados no Tick quando não houver mais players
		}
	}
}

void AMasterItem::ValidateItemData()
{
	// Validar Name
	if (Name.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AMasterItem: Name está vazio! Item será destruído."));
		Destroy();
		return;
	}

	// Validar Quantity
	if (Quantity <= 0)
	{
		Quantity = 1;
	}

	// Se não é stackable, força quantidade = 1
	if (!STQty.Stackable)
	{
		Quantity = 1;
	}
	else if (Quantity > STQty.MaxQty)
	{
		Quantity = STQty.MaxQty;
	}
}

FLinearColor AMasterItem::GetRarityColor() const
{
	switch (STInfos.Rarity)
	{
	case EItemRarity::Prototype:	// Amarelo
		return FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);
	case EItemRarity::Unstable:		// Verde
		return FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);
	case EItemRarity::Stable:		// Azul
		return FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);
	case EItemRarity::Enhanced:		// Branco
		return FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	case EItemRarity::Quantum:		// Laranja
		return FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);
	case EItemRarity::Singularity:	// Vermelho
		return FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
	default:
		return FLinearColor::White;
	}
}

void AMasterItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMasterItem, Name);
	DOREPLIFETIME(AMasterItem, ID);
	DOREPLIFETIME(AMasterItem, Quantity);
}
