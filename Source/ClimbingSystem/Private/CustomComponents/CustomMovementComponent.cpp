// Copyright INVI_1998, Inc. All Rights Reserved.


#include "CustomComponents/CustomMovementComponent.h"

#include "ClimbingSystem/DebugHelper.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TraceClimbableSurface();
}

void UCustomMovementComponent::TraceClimbableSurface() const
{
	// const FVector Start = GetOwner()->GetActorLocation();
	// const FVector End = Start + GetOwner()->GetActorForwardVector() * 100.0f;

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 50.0f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * 100.0f;

	TArray<FHitResult> HitResults = DoCapsuleTraceMultiByObject(Start, End, true);

	/*for (const FHitResult& HitResult : HitResults)
	{
		CS_Debug::Print("HitResult: " + HitResult.GetActor()->GetName());
	}*/
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
