// Copyright INVI_1998, Inc. All Rights Reserved.


#include "CustomComponents/CustomMovementComponent.h"

#include "ClimbingSystem/DebugHelper.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TraceClimbableSurface();

	TraceFromEyeHeight(100.f);
	
}

void UCustomMovementComponent::TraceClimbableSurface() const
{

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 50.0f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * 100.0f;

	TArray<FHitResult> HitResults = DoCapsuleTraceMultiByObject(Start, End, true);
}

void UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset) const
{
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);

	const FVector Start = ComponentLocation + EyeHeightOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

	TArray<FHitResult> HitResults = DoCapsuleTraceMultiByObject(Start, End, true);
}

TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebug) const
{
	TArray<FHitResult> HitResults;

	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this,
		Start,
		End,
		ClimbCapsuleTraceRadius,
		ClimbCapsuleTraceHalfHeight,
		ClimbTraceObjectTypes,
		false,
		TArray<AActor*>(),
		bShowDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		HitResults,
		false
	);


	return HitResults;

}

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebug) const
{
	FHitResult HitResult;

	UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		ClimbTraceObjectTypes,
		false,
		TArray<AActor*>(),
		bShowDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		HitResult,
		false
	);

	return HitResult;
}
