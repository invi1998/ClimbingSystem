// Copyright INVI_1998, Inc. All Rights Reserved.


#include "CustomComponents/CustomMovementComponent.h"

#include "MotionWarpingComponent.h"
#include "Character/CharacterAnimInstance.h"
#include "ClimbingSystem/DebugHelper.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UCustomMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();

	if (CharacterAnimInstance)
	{
		// 绑定攀爬蒙太奇结束事件
		CharacterAnimInstance->OnMontageEnded.AddDynamic(this, &UCustomMovementComponent::OnClimbMontageEnded);
		// 绑定攀爬蒙太奇淡出事件
		CharacterAnimInstance->OnMontageBlendingOut.AddDynamic(this, &UCustomMovementComponent::OnClimbMontageEnded);
	}

	ClimbingSystemCharacter = Cast<AClimbingSystemCharacter>(CharacterOwner);
}

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 如果角色移动速度大于0.1f
	if (Velocity.X > 10.0f || Velocity.Y > 10.f)
	{
		if (CharacterAnimInstance->IsAnyMontagePlaying())
		{
			// 如果有动画正在播放
			return;
		}

		if (CanStartClimbing())
		{
			// Start climbing
			PlayClimbMontage(AnimMontage_StandToWallUp);
		}
		else if (CanClimbDownLedge())
		{
			// 如果可以下爬，播放下爬蒙太奇
			PlayClimbMontage(AnimMontage_ClimbToDown);
		}
		else
		{
			TryStartVaulting();
		}
	}
	
}

void UCustomMovementComponent::ToggleClimbingMode(bool bEnableClimb)
{
	if (bEnableClimb)
	{
		// Enable climbing mode
		if (CanStartClimbing())
		{
			// Start climbing
			PlayClimbMontage(AnimMontage_StandToWallUp);
		}
		else if (CanClimbDownLedge())
		{
			// 如果可以下爬，播放下爬蒙太奇
			PlayClimbMontage(AnimMontage_ClimbToDown);
		}
		else
		{
			TryStartVaulting();
		}
	}
	else
	{
		// Disable climbing mode
		StopClimbing();
	}
}

bool UCustomMovementComponent::IsClimbing() const
{
	// 返回是否处于自定义移动模式，并且是攀爬模式
	return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
}

bool UCustomMovementComponent::CanStartClimbing()
{
	if (IsFalling())
	{
		// 如果角色正在下落，不允许开始攀爬
		return false;
	}

	if (IsClimbing())
	{
		// 如果角色正在攀爬，不允许开始攀爬
		return false;
	}

	if (!TraceClimbableSurface())
	{
		// 如果未检测到可攀爬表面，不允许开始攀爬
		return false;
	}

	if (!TraceFromEyeHeight(100.f).bBlockingHit)
	{
		// 如果未检测到攀爬顶端，不允许开始攀爬
		return false;
	}

	return true;

}

bool UCustomMovementComponent::CanClimbDownLedge() const
{
	if (IsFalling())
	{
		// 如果角色正在下落，不允许下爬
		return false;
	}

	if (IsClimbing())
	{
		// 如果角色正在攀爬，不允许下爬
		return false;
	}

	// 检测是否到达可下爬的地方

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	const FVector WalkableSurfaceTraceStart = ComponentLocation + ComponentForward * ClimbDownWalkableSurfaceTraceOffset;
	const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

	FHitResult WalkableSurfaceHitResult = DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, false, false);

	const FVector LedgeTraceStart = WalkableSurfaceHitResult.TraceStart + ComponentForward * ClimbDownLedgeTraceOffset;
	const FVector LedgeTraceEnd = LedgeTraceStart + DownVector * 300.f;

	FHitResult LedgeTraceHitResult = DoLineTraceSingleByObject(LedgeTraceStart, LedgeTraceEnd, false, false);

	if (WalkableSurfaceHitResult.bBlockingHit && !LedgeTraceHitResult.bBlockingHit)
	{
		// 如果检测到可下爬的地方，返回true
		return true;
	}

	return false;
}

void UCustomMovementComponent::StartClimbing()
{
	// 设置自定义移动模式为攀爬模式
	SetMovementMode(MOVE_Custom, ECustomMovementMode::MOVE_Climb);
}

void UCustomMovementComponent::StopClimbing()
{
	// 设置自定义移动模式为默认模式
	SetMovementMode(MOVE_Falling);
}

void UCustomMovementComponent::PhysClimb(float DeltaTime, int32 Iterations)
{
	// 该函数用于处理攀爬模式下的物理计算，在进入攀爬模式时会被每帧调用

	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// 处理攀爬表面
	TraceClimbableSurface();
	ProcessClimbableSurfaceInfo();

	// 检测是否应该攀爬
	if (!CheckShouldClimb() || CheckReachableGround())
	{
		// 如果不应该攀爬，停止攀爬
		// 如果到达地面，停止攀爬
		StopClimbing();
	}

	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		// 计算速度（传入的参数分别为：DeltaTime，水平速度，是否应用摩擦力，最大减速度）
		CalcVelocity(DeltaTime, 0.f, true, MaxBrakingDeceleration);
	}

	ApplyRootMotionToVelocity(DeltaTime);

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * DeltaTime;
	FHitResult Hit(1.f);

	// 处理攀爬旋转，使角色面向攀爬表面
	SafeMoveUpdatedComponent(Adjusted, GetClimbingRotation(DeltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		//adjust and try again
		HandleImpact(Hit, DeltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
	}

	// 将角色移动固定到攀爬表面
	SnapMovementToClimbableSurface(DeltaTime);

	// 
	if (CheckReachedLedge())
	{
		// 如果到达攀爬顶端，播放下墙蒙太奇
		PlayClimbMontage(AnimMontage_ClimbToTop);
	}
	
}

bool UCustomMovementComponent::CheckShouldClimb() const
{
	// 检查是否应该攀爬
	if (ClimbableSurfaceTraceHits.IsEmpty())
	{
		// 如果未检测到可攀爬表面，不应该攀爬
		return false;
	}

	//if (!TraceFromEyeHeight(100.f).bBlockingHit)
	//{
	//	// 如果未检测到攀爬顶端，不应该攀爬
	//	return false;
	//}

	// 计算当前攀爬表面法线和上向量的点积，然后计算角度差
	const float DotResult = FVector::DotProduct(CurrentClimbableSurfaceNormal, FVector::UpVector);
	const float DegreeDifference = FMath::RadiansToDegrees(FMath::Acos(DotResult));		// 计算角度差

	if (DegreeDifference <= 60.f)
	{
		// 如果角度差小于等于60度，不应该攀爬（表明角色已经攀爬到了一个太平的表面上）
		return false;
	}

	return true;
}

bool UCustomMovementComponent::CheckReachableGround() const
{
	// 检测是否到达地面
	const FVector DownVector = -UpdatedComponent->GetUpVector();
	const FVector StartOffset = DownVector * 50.0f;

	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + DownVector;

	TArray<FHitResult> HitResults = DoCapsuleTraceMultiByObject(Start, End, false, false);

	if (HitResults.IsEmpty())
	{
		// 如果未检测到地面，返回false
		return false;
	}

	for (const FHitResult& Hit : HitResults)
	{
		// 这里我们需要判断是可攀爬表面还是地面
		const bool bIsClimbableSurface = FVector::Parallel(-Hit.ImpactNormal, FVector::UpVector, 0.1f)		// 判断是否与上向量平行(0.1f是容差)
			&& GetUnRotatedClimbVelocity().Z <  -10.f;		// 判断是否在下落(速度小于-10.f)

		return bIsClimbableSurface;
	}

	return false;

}

bool UCustomMovementComponent::CheckReachedLedge() const
{
	// 检测是否到达攀爬顶端
	FHitResult EyeHeightHitResult = TraceFromEyeHeight(100.f, ClimbToTopTraceDistance);		// 从眼睛高度上方50.f开始检测

	if (EyeHeightHitResult.bBlockingHit)
	{
		// 如果检测到阻挡，说明还未到达攀爬顶端
		return false;
	}

	// 如果未检测到阻挡，说明到达攀爬顶端，此时在视角前方100.f的位置，再次检测垂直方向是否有阻挡
	const FVector WalkableSurfaceTraceStart = EyeHeightHitResult.TraceEnd;
	const FVector DownVector = -UpdatedComponent->GetUpVector();
	const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

	FHitResult WalkableSurfaceHitResult = DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, false, false);

	if (WalkableSurfaceHitResult.bBlockingHit && GetUnRotatedClimbVelocity().Z > 10.f)
	{
		// 如果检测到阻挡，说明到达了攀爬顶端，并且前方100.f的位置有地面，返回true
		// 并且速度大于10.f，表明角色正在向上攀爬
		return true;
	}

	return false;
}

void UCustomMovementComponent::ProcessClimbableSurfaceInfo()
{
	CurrentClimbableSurfaceLocation = FVector::ZeroVector;
	CurrentClimbableSurfaceNormal = FVector::ZeroVector;

	if (ClimbableSurfaceTraceHits.IsEmpty()) return;

	// 计算攀爬表面的位置和法线，取所有射线检测结果的平均值
	for (const FHitResult& Hit : ClimbableSurfaceTraceHits)
	{
		CurrentClimbableSurfaceLocation += Hit.ImpactPoint;
		CurrentClimbableSurfaceNormal += Hit.ImpactNormal;
	}

	CurrentClimbableSurfaceLocation /= ClimbableSurfaceTraceHits.Num();
	
	CurrentClimbableSurfaceNormal = CurrentClimbableSurfaceNormal.GetSafeNormal();

	//// Debug
	//UKismetSystemLibrary::DrawDebugSphere(this, CurrentClimbableSurfaceLocation, 10.f, 12, FColor::Green, 0.1f, 1.0f);
	//UKismetSystemLibrary::DrawDebugArrow(this, CurrentClimbableSurfaceLocation, CurrentClimbableSurfaceLocation + CurrentClimbableSurfaceNormal * 100.f, 10.f, FColor::Green, 0.1f, 1.0f);

}

FQuat UCustomMovementComponent::GetClimbingRotation(float DeltaTime) const
{
	// 计算攀爬旋转
	const FQuat CurrentRotation = UpdatedComponent->GetComponentQuat();		// 当前旋转

	// 如果我们想使用根运动来驱动旋转，或者根运动覆盖速度
	if (HasRootMotionSources() || CurrentRootMotion.HasOverrideVelocity())
	{
		// 如果有根运动源或根运动覆盖速度
		return CurrentRotation;
	}

	const FQuat TargetRotation = FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat();	// 目标旋转，根据攀爬表面法线计算，使角色面向攀爬表面（所以要对法线取反）

	// 通过插值计算旋转，使角色平滑旋转，避免瞬间旋转
	return FQuat::Slerp(CurrentRotation, TargetRotation, DeltaTime * 10.f);
}

void UCustomMovementComponent::SnapMovementToClimbableSurface(float DeltaTime)
{
	// 将角色移动固定到攀爬表面
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();

	// 计算角色面向攀爬表面的投影向量(投影到攀爬表面)
	const FVector ProjectedCharacterToSurface = (CurrentClimbableSurfaceLocation - ComponentLocation).ProjectOnTo(ComponentForward);

	// 计算角色面向攀爬表面的投影向量(投影到角色面向)
	const FVector SnapVector = -CurrentClimbableSurfaceNormal * ProjectedCharacterToSurface.Length();

	// 绘制 SnapVector
	// UKismetSystemLibrary::DrawDebugArrow(this, ComponentLocation, ComponentLocation + SnapVector, 10.f, FColor::Green, 0.1f, 1.0f);

	UpdatedComponent->MoveComponent(
		SnapVector*DeltaTime*MaxClimbSpeed,
		UpdatedComponent->GetComponentQuat(),
		true);
}

FVector UCustomMovementComponent::GetUnRotatedClimbVelocity() const
{
	// 获取未旋转的攀爬速度（因为四元数旋转的特性，所以要对速度进行反旋转，就能得到未旋转的速度）
	return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
}

void UCustomMovementComponent::ClimbDash()
{
	// 攀爬冲刺
	if (IsClimbing())
	{
		const FVector LastInputVector = GetLastInputVector().GetSafeNormal();
		// 获取最后一次输入向量，并对其进行反旋转，得到未旋转的输入向量，用于后续角色跳跃的方向判定
		// const FVector UnRotatedLastInputVector = UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), LastInputVector).GetSafeNormal();

		// 获取角色右向量
		const FVector RightVector = UpdatedComponent->GetRightVector().GetSafeNormal();
		// 获取角色上向量
		const FVector UpVector = UpdatedComponent->GetUpVector().GetSafeNormal();
		
		// UKismetSystemLibrary::DrawDebugArrow(this, UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + LastInputVector * 200, 10.f, FColor::Red, 1.1f, 1.0f);
		// UKismetSystemLibrary::DrawDebugArrow(this, UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + UnRotatedLastInputVector * 150.f, 10.f, FColor::Green, 1.1f, 1.0f);
		// UKismetSystemLibrary::DrawDebugArrow(this, UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + RightVector * 100.f, 10.f, FColor::Blue, 1.1f, 1.0f);
		// UKismetSystemLibrary::DrawDebugArrow(this, UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + UpVector * 100.f, 10.f, FColor::Yellow, 1.1f, 1.0f);

		const float DotResult_Z = FVector::DotProduct(LastInputVector, UpVector);
		// CS_Debug::Print("DotResult_Z: " + FString::SanitizeFloat(DotResult_Z));

		// const float AngleDifference = FMath::RadiansToDegrees(FMath::Acos(DotResult_Z));
		// CS_Debug::Print("AngleDifference: " + FString::SanitizeFloat(AngleDifference));

		if (DotResult_Z >= 0.9f)
		{
			// 如果输入向量与上向量的点积大于等于0.9f，表明角色正在向上攀爬
			HandleClimbDashUp();
		}
		else if (DotResult_Z <= -0.9f)
		{
			// 如果输入向量与上向量的点积小于等于-0.9f，表明角色正在向下攀爬
			HandleClimbDashDown();
		}
		else
		{
			// 计算和角色右向量的点积
			const float DotResult_X = FVector::DotProduct(LastInputVector, RightVector);
			// CS_Debug::Print("DotResult_X: " + FString::SanitizeFloat(DotResult_X));

			if (DotResult_X >= 0.9f)
			{
				// 如果输入向量与右向量的点积大于等于0.9f，表明角色正在向右攀爬
				HandleClimbDashRight();
			}
			else if (DotResult_X <= -0.9f)
			{
				// 如果输入向量与右向量的点积小于等于-0.9f，表明角色正在向左攀爬
				HandleClimbDashLeft();
			}
		}

		
	}
}

void UCustomMovementComponent::OnClimbMontageEnded(UAnimMontage* Montage, bool bBInterrupted)
{
	// 攀爬蒙太奇结束
	if (Montage == AnimMontage_StandToWallUp 
		|| Montage == AnimMontage_ClimbToDown 
		|| Montage == AnimMontage_ClimbDashUp
		|| Montage == AnimMontage_ClimbDashDown
		|| Montage == AnimMontage_ClimbDashLeft
		|| Montage == AnimMontage_ClimbDashRight)
	{
		if (!bBInterrupted)
		{
			// 如果是上墙蒙太奇被打断，不继续攀爬
			StartClimbing();
		}
	}
	else if (Montage == AnimMontage_ClimbToTop || Montage == AnimMontage_Vaulting)
	{
		// 如果是上到顶端蒙太奇结束
		SetMovementMode(MOVE_Walking);
	}
}

void UCustomMovementComponent::PlayClimbMontage(UAnimMontage* MontageToPlay)
{
	if (CharacterAnimInstance && MontageToPlay)
	{
		if (CharacterAnimInstance->Montage_IsPlaying(MontageToPlay))
		{
			// 如果动画正在播放，不重复播放
			return;
		}
		if (CharacterAnimInstance->IsAnyMontagePlaying())
		{
			// 如果有动画正在播放，停止播放
			return;
		}
		
		CharacterAnimInstance->Montage_Play(MontageToPlay);
	}
}

void UCustomMovementComponent::TryStartVaulting()
{
	FVector VaultStartLocation = FVector::ZeroVector;
	FVector VaultLandLocation = FVector::ZeroVector;
	if (CanStartVaulting(VaultStartLocation, VaultLandLocation))
	{
		// 如果可以开始翻越
		// Start vaulting
		// UKismetSystemLibrary::DrawDebugSphere(this, VaultStartLocation, 10.f, 12, FColor::Green, 0.1f, 1.0f);
		// UKismetSystemLibrary::DrawDebugSphere(this, VaultLandLocation, 10.f, 12, FColor::Blue, 0.1f, 1.0f);

		SetMotionWarpingTarget("VaultStartPoint", VaultStartLocation);
		SetMotionWarpingTarget("VaultEndPoint", VaultLandLocation);

		StartClimbing();
		PlayClimbMontage(AnimMontage_Vaulting);
	}
}

bool UCustomMovementComponent::CanStartVaulting(FVector& OutVaultStartLocation, FVector& OutVaultLandLocation) const
{
	if (IsClimbing())
	{
		// 如果正在攀爬，不允许翻越
		return false;
	}
	if (IsFalling())
	{
		// 如果正在下落，不允许翻越
		return false;
	}

	if (TraceFromEyeHeight(100.f).bBlockingHit)
	{
		// 如果当前视线高度100.f位置有阻挡，不允许翻越
		return false;
	}

	OutVaultStartLocation = FVector::ZeroVector;
	OutVaultLandLocation = FVector::ZeroVector;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector UpVector = UpdatedComponent->GetUpVector();
	const FVector DownVector = -UpVector;

	for (int32 i = 0; i < 5; i++)
	{
		const FVector Start = ComponentLocation + ComponentForward * 100.f * (i+1) + UpVector * 100.f;
		const FVector End = Start + DownVector * 100.f * (i+1);

		FHitResult HitResult = DoLineTraceSingleByObject(Start, End, false, false);

		if (i == 0 && HitResult.bBlockingHit)
		{
			// 如果第一次检测到阻挡，说明角色前方0位置可以作为翻越起点
			OutVaultStartLocation = HitResult.ImpactPoint;
		}

		if (i == 3 && HitResult.bBlockingHit)
		{
			// 如果第五次检测到阻挡，说明角色前方100位置可以作为翻越终点
			OutVaultLandLocation = HitResult.ImpactPoint;
		}
	}

	if (OutVaultStartLocation != FVector::ZeroVector && OutVaultLandLocation != FVector::ZeroVector)
	{
		// 如果检测到翻越起点和终点，返回true
		return true;
	}

	return false;

}

void UCustomMovementComponent::SetMotionWarpingTarget(const FName& TargetSectionName, const FVector& TargetLocation) const
{
	if (ClimbingSystemCharacter)
	{
		ClimbingSystemCharacter->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocation(TargetSectionName, TargetLocation);
	}
}

void UCustomMovementComponent::HandleClimbDashUp()
{
	FVector TargetPoint = FVector::ZeroVector;
	if (CheckClimbDashUp(TargetPoint))
	{
		SetMotionWarpingTarget("DashUpTargetPoint", TargetPoint);
		// 如果可以攀爬冲刺上
		PlayClimbMontage(AnimMontage_ClimbDashUp);
	}
}

bool UCustomMovementComponent::CheckClimbDashUp(FVector& OutTargetPoint)
{
	OutTargetPoint = FVector::ZeroVector;

	if (IsFalling())
	{
		// 如果正在下落，不允许攀爬冲刺上
		return false;
	}

	FHitResult HitResult = TraceFromEyeHeight(100.f, -30.f, false, true);
	FHitResult SaftyLedgeHitResult = TraceFromEyeHeight(100.f, 150.f, false, true);
	if (HitResult.bBlockingHit && SaftyLedgeHitResult.bBlockingHit)
	{
		// 如果检测到阻挡，并且检测到安全的突出物，允许攀爬冲刺上
		OutTargetPoint = HitResult.ImpactPoint;
		return true;
	}

	return false;
}

void UCustomMovementComponent::HandleClimbDashDown()
{
	FVector TargetPoint = FVector::ZeroVector;
	if (CheckClimbDashDown(TargetPoint))
	{
		SetMotionWarpingTarget("DashDownTargetPoint", TargetPoint);
		// 如果可以攀爬冲刺下
		PlayClimbMontage(AnimMontage_ClimbDashDown);
	}
}

bool UCustomMovementComponent::CheckClimbDashDown(FVector& OutTargetPoint)
{
	OutTargetPoint = FVector::ZeroVector;

	if (IsFalling())
	{
		// 如果正在下落，不允许攀爬冲刺下
		return false;
	}

	FHitResult HitResult = TraceFromEyeHeight(100.f, -300.f, false, true);
	if (HitResult.bBlockingHit)
	{
		// 如果检测到阻挡，允许攀爬冲刺下
		OutTargetPoint = HitResult.ImpactPoint;
		return true;
	}

	return false;
}

void UCustomMovementComponent::HandleClimbDashLeft()
{
	FVector TargetPoint = FVector::ZeroVector;
	if (CheckClimbDashLeft(TargetPoint))
	{
		SetMotionWarpingTarget("DashLeftTargetPoint", TargetPoint);
		// 如果可以攀爬冲刺左
		PlayClimbMontage(AnimMontage_ClimbDashLeft);
	}
}

bool UCustomMovementComponent::CheckClimbDashLeft(FVector& OutTargetPoint)
{
	OutTargetPoint = FVector::ZeroVector;

	if (IsFalling())
	{
		// 如果正在下落，不允许攀爬冲刺左
		return false;
	}

	FHitResult HitResult = TraceFromEyeHeight_V(100.f, -200.f, false, true);
	if (HitResult.bBlockingHit)
	{
		// 如果检测到阻挡，允许攀爬冲刺左
		OutTargetPoint = HitResult.ImpactPoint;
		return true;
	}

	return false;
}

void UCustomMovementComponent::HandleClimbDashRight()
{
	FVector TargetPoint = FVector::ZeroVector;
	if (CheckClimbDashRight(TargetPoint))
	{
		SetMotionWarpingTarget("DashRightTargetPoint", TargetPoint);
		// 如果可以攀爬冲刺右
		PlayClimbMontage(AnimMontage_ClimbDashRight);
	}
}

bool UCustomMovementComponent::CheckClimbDashRight(FVector& OutTargetPoint)
{
	OutTargetPoint = FVector::ZeroVector;

	if (IsFalling())
	{
		// 如果正在下落，不允许攀爬冲刺右
		return false;
	}

	FHitResult HitResult = TraceFromEyeHeight_V(100.f, 200.f, false, true);
	if (HitResult.bBlockingHit)
	{
		// 如果检测到阻挡，允许攀爬冲刺右
		OutTargetPoint = HitResult.ImpactPoint;
		return true;
	}

	return false;
}

void UCustomMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{

	if (IsClimbing())
	{
		// 如果进入攀爬模式
		bOrientRotationToMovement = false;	// 不根据移动方向旋转角色
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(CharacterCapsuleHalfHeight/2.f);	// 设置胶囊体高度

		OnEnterClimbState_Delegate.ExecuteIfBound();	// 触发进入攀爬状态委托
	}
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
	{
		// 如果离开攀爬模式
		bOrientRotationToMovement = true;	// 根据移动方向旋转角色
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(CharacterCapsuleHalfHeight);	// 恢复胶囊体高度

		// 重置角色旋转，只保留Yaw旋转，Pitch和Roll重置为0，因为在攀爬模式下，角色的Pitch和Roll会因为身体紧贴墙面导致发生变化
		const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
		const FRotator CleanRotation = FRotator(0.f, DirtyRotation.Yaw, 0.f);		// 重置角色旋转
		UpdatedComponent->SetRelativeRotation(CleanRotation);		// 设置角色旋转

		StopMovementImmediately();		// 停止移动

		OnExitClimbState_Delegate.ExecuteIfBound();	// 触发退出攀爬状态委托
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UCustomMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (IsClimbing())
	{
		// 如果处于攀爬模式
		PhysClimb(DeltaTime, Iterations);
	}

	Super::PhysCustom(DeltaTime, Iterations);
}

float UCustomMovementComponent::GetMaxSpeed() const
{
	if (IsClimbing())
	{
		// 如果处于攀爬模式，返回攀爬速度
		return MaxClimbSpeed;
	}
	return Super::GetMaxSpeed();
}

float UCustomMovementComponent::GetMaxAcceleration() const
{
	if (IsClimbing())
	{
		// 如果处于攀爬模式，返回攀爬加速度
		return MaxClimbAcceleration;
	}
	return Super::GetMaxAcceleration();
}

FVector UCustomMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const
{
	// 这里之所以重写这个函数，是因为我们需要在攀爬模式下，限制根动作的速度，使角色在攀爬时不会因为根动作速度过大而脱离攀爬表面
	// 同时，在父类中，这个函数会判断角色是否处于掉落状态，如果是，会将根动作速度限制为0，这样会导致角色在攀爬时无法播放根动作
	if (IsFalling() && CharacterAnimInstance && CharacterAnimInstance->IsAnyMontagePlaying())
	{
		// 如果角色处于掉落状态，并且有动画蒙太奇正在播放
		return RootMotionVelocity;
	}
	else
	{
		return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
	}
}

bool UCustomMovementComponent::TraceClimbableSurface()
{

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 30.0f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector();

	ClimbableSurfaceTraceHits = DoCapsuleTraceMultiByObject(Start, End, false);

	return !ClimbableSurfaceTraceHits.IsEmpty();
}

FHitResult UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset, bool bShowDebug, bool bDrawPersistantShapes) const
{
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);

	const FVector Start = ComponentLocation + EyeHeightOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineTraceSingleByObject(Start, End, bShowDebug, bDrawPersistantShapes);
}

FHitResult UCustomMovementComponent::TraceFromEyeHeight_V(float TraceDistance, float TraceStartOffset, bool bShowDebug, bool bDrawPersistantShapes) const
{
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetRightVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);

	const FVector Start = ComponentLocation + EyeHeightOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineTraceSingleByObject(Start, End, bShowDebug, bDrawPersistantShapes);
}

TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebug, bool bDrawPersistantShapes) const
{
	TArray<FHitResult> HitResults;

	const EDrawDebugTrace::Type DebugTraceType = bDrawPersistantShapes ? EDrawDebugTrace::Persistent : EDrawDebugTrace::ForDuration;

	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this,
		Start,
		End,
		ClimbCapsuleTraceRadius,
		ClimbCapsuleTraceHalfHeight,
		ClimbTraceObjectTypes,
		false,
		TArray<AActor*>(),
		bShowDebug ? DebugTraceType : EDrawDebugTrace::None,
		HitResults,
		false
	);


	return HitResults;

}

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebug, bool bDrawPersistantShapes) const
{
	FHitResult HitResult;

	const EDrawDebugTrace::Type DebugTraceType = bDrawPersistantShapes ? EDrawDebugTrace::Persistent : EDrawDebugTrace::ForDuration;

	UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		ClimbTraceObjectTypes,
		false,
		TArray<AActor*>(),
		bShowDebug ? DebugTraceType : EDrawDebugTrace::None,
		HitResult,
		false
	);

	return HitResult;
}


