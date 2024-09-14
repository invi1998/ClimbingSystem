// Copyright INVI_1998, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

class UAnimMontage;
class UAnimInstance;

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

	FORCEINLINE FVector GetCurrentClimbableSurfaceLocation() const { return CurrentClimbableSurfaceLocation; }
	FORCEINLINE FVector GetCurrentClimbableSurfaceNormal() const { return CurrentClimbableSurfaceNormal; }

protected:
	UFUNCTION()
	void OnClimbMontageEnded(UAnimMontage* Montage, bool bBInterrupted);		// 攀爬蒙太奇结束

	virtual void BeginPlay() override;

	// 重写TickComponent
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 重写移动模式改变
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	// 重写物理计算
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	// 重写获取最大速度
	virtual float GetMaxSpeed() const override;

	// 重写获取最大加速度
	virtual float GetMaxAcceleration() const override;

private:
	void StartClimbing();

	void StopClimbing();

	void PhysClimb(float DeltaTime, int32 Iterations);

	// 检查是否应该攀爬
	bool CheckShouldClimb() const;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float MaxBrakingDeceleration = 400.f;	// 最大减速度

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float MaxClimbSpeed = 200.f;	// 最大攀爬速度

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float MaxClimbAcceleration = 500.f;	// 最大攀爬加速度

	void ProcessClimbableSurfaceInfo();

	// 获取攀爬旋转
	FQuat GetClimbingRotation(float DeltaTime) const;

	// 将移动位置对齐到可攀爬表面
	void SnapMovementToClimbableSurface(float DeltaTime);

	FVector CurrentClimbableSurfaceLocation;	// 当前可攀爬表面的位置
	FVector CurrentClimbableSurfaceNormal;		// 当前可攀爬表面的法线

	UPROPERTY()
	UAnimInstance* CharacterAnimInstance = nullptr;

	void PlayClimbMontage(UAnimMontage* MontageToPlay);

	/**
	 * UnGrabWall Montages （未抓住墙壁动画，上墙和下墙）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing: Montage", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_StandToWallUp = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing: Montage", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_WallDownToStand = nullptr;


	
};
