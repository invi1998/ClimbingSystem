// Copyright INVI_1998, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimInstance.generated.h"

class AClimbingSystemCharacter;
class UCustomMovementComponent;

/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;		// 在动画实例初始化时调用，相当于游戏中的BeginPlay

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;	// 在动画实例更新时调用，相当于游戏中的Tick


private:
	UPROPERTY()
	AClimbingSystemCharacter* ClimbingSystemCharacter;

	UPROPERTY()
	UCustomMovementComponent* CustomMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference, meta = (AllowPrivateAccess = "true"))
	float GroundSpeed;		// 地面速度
	void GetGroundSpeed();	// 获取地面速度

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference, meta = (AllowPrivateAccess = "true"))
	float AirSpeed;			// 空中速度
	void GetAirSpeed();		// 获取空中速度

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference, meta = (AllowPrivateAccess = "true"))
	bool bShouldMove;		// 是否应该移动
	void GetShouldMove();	// 获取是否应该移动

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference, meta = (AllowPrivateAccess = "true"))
	bool bIsFalling;		// 是否在下落
	void GetIsFalling();	// 获取是否在下落

	
};
