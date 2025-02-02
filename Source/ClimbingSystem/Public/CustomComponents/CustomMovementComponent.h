// Copyright INVI_1998, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

DECLARE_DELEGATE(FOnEnterClimbState)
DECLARE_DELEGATE(FOnExitClimbState)

class UAnimMontage;
class UCharacterAnimInstance;
class AClimbingSystemCharacter;

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

	// 是否可以下爬
	bool CanClimbDownLedge() const;

	FORCEINLINE FVector GetCurrentClimbableSurfaceLocation() const { return CurrentClimbableSurfaceLocation; }
	FORCEINLINE FVector GetCurrentClimbableSurfaceNormal() const { return CurrentClimbableSurfaceNormal; }

	FVector GetUnRotatedClimbVelocity() const;	// 获取未旋转的攀爬速度

	FOnEnterClimbState OnEnterClimbState_Delegate;		// 进入攀爬状态委托
	FOnExitClimbState OnExitClimbState_Delegate;		// 退出攀爬状态委托

	void ClimbDash();	// 攀爬冲刺

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

	// 重写根动作约束，用于处理攀爬时的根动作
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;

private:
	void StartClimbing();

	void StopClimbing();

	void PhysClimb(float DeltaTime, int32 Iterations);

	// 检查是否应该攀爬
	bool CheckShouldClimb() const;

	// 检测是否到达地面
	bool CheckReachableGround() const;

	// 检测是否到达攀爬顶端
	bool CheckReachedLedge() const;

	// 跟踪可攀爬表面
	bool TraceClimbableSurface();

	// 从眼睛高度开始跟踪
	FHitResult TraceFromEyeHeight(float TraceDistance, float TraceStartOffset = 0.f, bool bShowDebug = false, bool bDrawPersistantShapes = false) const;

	FHitResult TraceFromEyeHeight_V(float TraceDistance, float TraceStartOffset = 0.f, bool bShowDebug = false, bool bDrawPersistantShapes = false) const;

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
	float MaxClimbSpeed = 100.f;	// 最大攀爬速度

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float MaxClimbAcceleration = 200.f;	// 最大攀爬加速度

	// 夹角阈值（收拢下面这两个参数，可以让角色的下落检测更严格，避免角色在下落时朝向过于错误）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float ClimbDownWalkableSurfaceTraceOffset = 25.f;	// 攀爬下行走表面射线检测偏移

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float ClimbDownLedgeTraceOffset = 18.f;	// 攀爬下行走表面射线检测偏移

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float CharacterCapsuleHalfHeight = 94.f;	// 角色胶囊体高度

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	float ClimbToTopTraceDistance = 10.f;	// 攀爬到顶端射线检测距离

	void ProcessClimbableSurfaceInfo();

	// 获取攀爬旋转
	FQuat GetClimbingRotation(float DeltaTime) const;

	// 将移动位置对齐到可攀爬表面
	void SnapMovementToClimbableSurface(float DeltaTime);

	FVector CurrentClimbableSurfaceLocation;	// 当前可攀爬表面的位置
	FVector CurrentClimbableSurfaceNormal;		// 当前可攀爬表面的法线

	UPROPERTY()
	UAnimInstance* CharacterAnimInstance;

	UPROPERTY()
	AClimbingSystemCharacter* ClimbingSystemCharacter;

	void PlayClimbMontage(UAnimMontage* MontageToPlay);

	/**
	 * UnGrabWall Montages （未抓住墙壁动画，上墙和下墙）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_StandToWallUp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_ClimbToTop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_ClimbToDown;

	void TryStartVaulting();	// 尝试开始翻越

	bool CanStartVaulting(FVector& OutVaultStartLocation, FVector& OutVaultLandLocation) const;	// 是否可以开始翻越，返回翻越起始位置和落地位置

	void SetMotionWarpingTarget(const FName& TargetSectionName, const FVector& TargetLocation) const;	// 设置翻越运动扭曲目标

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_Vaulting;

	void HandleClimbDashUp();	// 处理攀爬冲刺上

	bool CheckClimbDashUp(FVector& OutTargetPoint);	// 检查攀爬冲刺上

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_ClimbDashUp;

	void HandleClimbDashDown();	// 处理攀爬冲刺下
	bool CheckClimbDashDown(FVector& OutTargetPoint);	// 检查攀爬冲刺下

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_ClimbDashDown;

	void HandleClimbDashLeft();	// 处理攀爬冲刺左
	bool CheckClimbDashLeft(FVector& OutTargetPoint);	// 检查攀爬冲刺左

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_ClimbDashLeft;

	void HandleClimbDashRight();	// 处理攀爬冲刺右
	bool CheckClimbDashRight(FVector& OutTargetPoint);	// 检查攀爬冲刺右

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing", meta=(AllowPrivateAccess = "true"))
	UAnimMontage* AnimMontage_ClimbDashRight;

};
