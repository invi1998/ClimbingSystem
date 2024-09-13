// Copyright INVI_1998, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 跟踪可攀爬表面
	void TraceClimbableSurface() const;

	// 从眼睛高度开始跟踪
	void TraceFromEyeHeight(float TraceDistance, float TraceStartOffset = 0.f) const;

	// 胶囊体射线检测
	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebug) const;

	// 线性射线检测 (单个，用于检测是否达到攀爬顶端）
	FHitResult DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebug) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceRadius = 50.0f;		// 胶囊体射线检测半径

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceHalfHeight = 72.0f;	// 胶囊体射线检测高度

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	TArray<TEnumAsByte<EObjectTypeQuery>> ClimbTraceObjectTypes;	// 胶囊体射线检测对象类型
	
};
