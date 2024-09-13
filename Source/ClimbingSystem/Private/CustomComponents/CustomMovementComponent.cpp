// Copyright INVI_1998, Inc. All Rights Reserved.


#include "CustomComponents/CustomMovementComponent.h"

#include "ClimbingSystem/DebugHelper.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "Components/CapsuleComponent.h"

void UCustomMovementComponent::ToggleClimbingMode(bool bEnableClimb)
{
	if (bEnableClimb)
	{
		// Enable climbing mode
		if (CanStartClimbing())
		{
			// Start climbing
			CS_Debug::Print("Start climbing", FColor::Green);
			StartClimbing();
		}
		else
		{
			CS_Debug::Print("Can't start climbing", FColor::Red);
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

	// Debug
	UKismetSystemLibrary::DrawDebugSphere(this, CurrentClimbableSurfaceLocation, 10.f, 12, FColor::Green, 0.1f, 1.0f);
	UKismetSystemLibrary::DrawDebugArrow(this, CurrentClimbableSurfaceLocation, CurrentClimbableSurfaceLocation + CurrentClimbableSurfaceNormal * 100.f, 10.f, FColor::Green, 0.1f, 1.0f);

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

	UpdatedComponent->MoveComponent(
		SnapVector*DeltaTime*MaxClimbSpeed,
		UpdatedComponent->GetComponentQuat(),
		true);
}

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

void UCustomMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{

	if (IsClimbing())
	{
		// 如果进入攀爬模式
		bOrientRotationToMovement = false;	// 不根据移动方向旋转角色
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(45.0f);	// 设置胶囊体高度
	}
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
	{
		// 如果离开攀爬模式
		bOrientRotationToMovement = true;	// 根据移动方向旋转角色
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(90.0f);	// 恢复胶囊体高度

		// 重置角色旋转，只保留Yaw旋转，Pitch和Roll重置为0，因为在攀爬模式下，角色的Pitch和Roll会因为身体紧贴墙面导致发生变化
		const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
		const FRotator CleanRotation = FRotator(0.f, DirtyRotation.Yaw, 0.f);		// 重置角色旋转
		UpdatedComponent->SetRelativeRotation(CleanRotation);		// 设置角色旋转

		StopMovementImmediately();		// 停止移动
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

bool UCustomMovementComponent::TraceClimbableSurface()
{

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 30.0f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector();

	ClimbableSurfaceTraceHits = DoCapsuleTraceMultiByObject(Start, End, true);

	return !ClimbableSurfaceTraceHits.IsEmpty();
}

FHitResult UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset) const
{
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);

	const FVector Start = ComponentLocation + EyeHeightOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineTraceSingleByObject(Start, End, true, true);
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


