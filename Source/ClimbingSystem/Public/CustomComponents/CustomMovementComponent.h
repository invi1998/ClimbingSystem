// Copyright INVI_1998, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

UENUM(BlueprintType)
namespace ECustomMovementMode
{
	enum Type
	{
		MOVE_Climb UMETA(DisplayName = "Climb Mode"),
	};
}

/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	// 设置是否开启攀爬模式
	void ToggleClimbingMode(bool bEnableClimb);
	bool IsClimbing() const;

	// 是否可以开始攀爬
	bool CanStartClimbing();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

private:
	void StartClimbing();

	void StopClimbing();

	// 跟踪可攀爬表面
	bool TraceClimbableSurface();

	// 从眼睛高度开始跟踪
	FHitResult TraceFromEyeHeight(float TraceDistance, float TraceStartOffset = 0.f) const;

	// 胶囊体射线检测
	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebug, bool bDrawPersistantShapes = false) const;

	// 线性射线检测 (单个，用于检测是否达到攀爬顶端）
	FHitResult DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebug, bool bDrawPersistantShapes = false) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceRadius = 50.0f;		// 胶囊体射线检测半径

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceHalfHeight = 72.0f;	// 胶囊体射线检测高度

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	TArray<TEnumAsByte<EObjectTypeQuery>> ClimbTraceObjectTypes;	// 胶囊体射线检测对象类型

	TArray<FHitResult> ClimbableSurfaceTraceHits;	// 可攀爬表面的射线检测结果
	
};
