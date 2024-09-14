// Copyright INVI_1998, Inc. All Rights Reserved.


#include "Character/CharacterAnimInstance.h"

#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "ClimbingSystem/DebugHelper.h"
#include "CustomComponents/CustomMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ClimbingSystemCharacter = Cast<AClimbingSystemCharacter>(TryGetPawnOwner());
	if (ClimbingSystemCharacter)
	{
		CustomMovementComponent = ClimbingSystemCharacter->GetCustomMovementComponent();
	}
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!ClimbingSystemCharacter || !CustomMovementComponent) return;

	GetGroundSpeed();
	GetAirSpeed();
	GetIsFalling();
	GetShouldMove();
	GetIsClimbing();
	GetClimbVelocity();
}

void UCharacterAnimInstance::GetGroundSpeed()
{
	if (ClimbingSystemCharacter)
	{
		// UKismetMathLibrary::VSize(ClimbingSystemCharacter->GetVelocity()); == ClimbingSystemCharacter->GetVelocity().Size();
		GroundSpeed = UKismetMathLibrary::VSizeXY(ClimbingSystemCharacter->GetVelocity());
	}
	else
	{
		GroundSpeed = 0.0f;
	}
}

void UCharacterAnimInstance::GetAirSpeed()
{
	if (ClimbingSystemCharacter)
	{
		AirSpeed = ClimbingSystemCharacter->GetVelocity().Z;
	}
	else
	{
		AirSpeed = 0.0f;
	}
}

void UCharacterAnimInstance::GetShouldMove()
{
	if (CustomMovementComponent)
	{
		bShouldMove = CustomMovementComponent->GetCurrentAcceleration().Size() > 0.0f
			&& GroundSpeed > 5.0f
			&& !bIsFalling;
	}
	else
	{
		bShouldMove = false;
	}

}

void UCharacterAnimInstance::GetIsFalling()
{
	if (CustomMovementComponent)
	{
		bIsFalling = CustomMovementComponent->IsFalling();
	}
	else
	{
		bIsFalling = false;
	}
}

void UCharacterAnimInstance::GetIsClimbing()
{
	if (CustomMovementComponent)
	{
		bIsClimbing = CustomMovementComponent->IsClimbing();
	}
	else
	{
		bIsClimbing = false;
	}
}

void UCharacterAnimInstance::GetClimbVelocity()
{
	if (CustomMovementComponent)
	{
		ClimbVelocity = CustomMovementComponent->GetUnRotatedClimbVelocity();
	}
	else
	{
		ClimbVelocity = FVector::ZeroVector;
	}
}
