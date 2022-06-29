// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if (BlasterCharacter == nullptr) return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bIsAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bElimmed = BlasterCharacter->IsElimmed();

	// Offset Yaw for Strafing
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		// 世界坐标系的插槽Transform
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		// 转到Weapon的本地坐标系
		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		// 控制只有本地端才调整，节省带宽
		if (BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
			
			// 插槽Hand_R是朝内的，所以方向为插槽位置指向，插槽到目标点的方向...插值平滑过渡
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation()+ RightHandTransform.GetLocation()-BlasterCharacter->GetHitTarget());
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);

			// DrawDebugLine(GetWorld(), RightHandTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Green);


		}

		bUseFABRIK = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
		bUseAimOffsets = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
		bTransformRightHand = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;


// 		/*
// 		* 调试用来画射线的，查看枪口方向与目标方向的偏差
// 		*/ 
// 		FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
// 		// 获得X方向向量
// 		FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
// 		// 枪口方向射线
// 		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
// 		// 枪口到目标射线
// 		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Orange);
	}

}
