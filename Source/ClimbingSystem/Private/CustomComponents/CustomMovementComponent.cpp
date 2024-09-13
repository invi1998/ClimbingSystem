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

		StopMovementImmediately();		// 停止移动
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UCustomMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	Super::PhysCustom(DeltaTime, Iterations);
}

bool UCustomMovementComponent::TraceClimbableSurface()
{

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 30.0f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector();

	ClimbableSurfaceTraceHits = DoCapsuleTraceMultiByObject(Start, End, true, true);

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
