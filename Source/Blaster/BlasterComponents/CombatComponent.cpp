#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
// #include "Blaster/HUD/BlasterHUD.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 350.f;
}


void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);		// Я���ӵ�ֻ���Լ��й�
	DOREPLIFETIME(UCombatComponent, CombatState);

}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}

}


void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	// ��������Ŀ�����ߵ�
	if (Character && Character->IsLocallyControlled())
	{
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;	
		
		SetHUDCorsshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}

}

void UCombatComponent::SetHUDCorsshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
			}

			// calculate crosshair spread���ٶ�Խ��CrosshairVelocityFactor�������CrosshairInAirFactor���2.25�������Ѹ�ٱ��0
			
			// [0,600]->[0,1]
			float MaxSpeed;
			if (Character->bIsCrouched)
			{
				MaxSpeed = Character->GetCharacterMovement()->MaxWalkSpeedCrouched;
			}
			else
			{
				MaxSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
			}

			FVector2D SpeedRange(0.f, MaxSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(SpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.48f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}
			if (HitResult.GetActor() && HitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
			{
				CrosshairAimAtAnotherPlayerFactor = FMath::FInterpTo(CrosshairAimFactor, 0.2f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimAtAnotherPlayerFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			// �������Ѹ�ٹ���
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 10.f);
			
			
			// ��׼ֵ��������̫С// ���ű�ʾ��׼ʱ׼�ı�С
			HUDPackage.CrosshairSpread = .5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor - CrosshairAimAtAnotherPlayerFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	if (Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	// ������ǹ�Ļ������ȶ���
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();			// ��ǹ֮�������ʾ

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		UE_LOG(LogTemp, Error, TEXT("%d"), CarriedAmmo);
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}

	// �Զ�װ��
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}

	// ��ǹ֮��ʹ��controller����ת����ʹ���Լ�����ת
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(-ReloadAmount);
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		// ��������Ϊ�˷�ֹAttachActor��EquippedWeapon����֮ǰ�͵�����
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		// ��ǹ֮��ʹ��controller����ת����ʹ���Լ�����ת
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}

	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}

}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	
	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);

		if (EquippedWeapon)
		{
			CrosshairShootingFactor = .75f;
		}
		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,		// ��ʱ�����ִ�еĺ���
		EquippedWeapon->FireDelay					// �ӳ�ʱ��
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}

	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	// ��ȡ�ӿڳߴ�
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// ��ȡʮ��׼�ĵ�λ�ú���׼����
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);		// �ӿ�����

	FVector CrosshairworldPosition;			// ��������׼����������
	FVector CrosshairworldDirection;		// ����������׼������������
	// �ӿ�����ת�������꣬�����Ƿ�ת���ɹ���boolֵ
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairworldPosition,
		CrosshairworldDirection
	);

	if (bScreenToWorld)
	{
		// ��׼���ߵ������յ㣨δ���У����յ�Ϊ���������׼����һ������
		FVector Start = CrosshairworldPosition;

		// ������ƶ�����ɫ֮ǰ
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairworldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairworldDirection * TRACE_LENGTH;

		// ����׷��
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_PhysicsBody					// �ǵõ�����ײͨ��
		);

		// �ж����߼�⵽��Ŀ���Ƿ�ʵ����UInteractWithCrosshairsInterface�ӿ�
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}

		// �������û����,�򽫻��е�����Ϊ�����յ�
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		else
		{
// 			// �����ˣ��ͻ���һ��������������
// 			DrawDebugSphere(
// 				GetWorld(),
// 				TraceHitResult.ImpactPoint,
// 				12.f,
// 				12,
// 				FColor::Red
// 			);
		}
	}
}

