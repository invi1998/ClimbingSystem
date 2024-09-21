// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingSystemCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "CustomComponents/CustomMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MotionWarpingComponent.h"
#include "DebugHelper.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AClimbingSystemCharacter

AClimbingSystemCharacter::AClimbingSystemCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CustomMovementComponent = Cast<UCustomMovementComponent>(GetCharacterMovement());

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Motion Warping Component（运动扭曲组件）
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

}

void AClimbingSystemCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	AddInputMappingContext(DefaultMappingContext, 0);

	if (CustomMovementComponent)
	{
		CustomMovementComponent->OnEnterClimbState_Delegate.BindUObject(this, &AClimbingSystemCharacter::OnPlayerEnterClimbState);
		CustomMovementComponent->OnExitClimbState_Delegate.BindUObject(this, &AClimbingSystemCharacter::OnPlayerExitClimbState);
	}

}

void AClimbingSystemCharacter::ClimbDash(const FInputActionValue& Value)
{
	if (!CustomMovementComponent) return;

	CustomMovementComponent->ClimbDash();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AClimbingSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AClimbingSystemCharacter::HandleGroundMovementInput);

		// Climbing Moving
		EnhancedInputComponent->BindAction(ClimbMoveAction, ETriggerEvent::Triggered, this, &AClimbingSystemCharacter::HandleClimbingMovementInput);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AClimbingSystemCharacter::Look);

		// Climbing
		EnhancedInputComponent->BindAction(ClimbingAction, ETriggerEvent::Started, this, &AClimbingSystemCharacter::Climbing);

		// Climbing Dash
		EnhancedInputComponent->BindAction(ClimbDashAction, ETriggerEvent::Started, this, &AClimbingSystemCharacter::ClimbDash);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AClimbingSystemCharacter::OnPlayerEnterClimbState()
{
	AddInputMappingContext(ClimbingMappingContext, 1);
}

void AClimbingSystemCharacter::OnPlayerExitClimbState()
{
	RemoveInputMappingContext(ClimbingMappingContext);
}

void AClimbingSystemCharacter::AddInputMappingContext(UInputMappingContext* MappingContext, int32 InPriority)
{
	if (!MappingContext) return;
	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, InPriority);
		}
	}
}

void AClimbingSystemCharacter::RemoveInputMappingContext(UInputMappingContext* MappingContext)
{
	if (!MappingContext) return;
	//Remove Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(MappingContext);
		}
	}
}

void AClimbingSystemCharacter::HandleGroundMovementInput(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AClimbingSystemCharacter::HandleClimbingMovementInput(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 使用攀爬表面法线和角色右向量的叉乘来计算角色的前向向量（因为是在墙面攀爬，所以这里角色的移动的前向向量就是上下方向）,同时，注意这里的法线是反的，所以要取反
		const FVector ForwardDirection = FVector::CrossProduct(-CustomMovementComponent->GetCurrentClimbableSurfaceNormal(), GetActorRightVector()).GetSafeNormal();

		// 使用攀爬表面法线和角色上向量的叉乘来计算角色的右向量
		const FVector RightDirection = FVector::CrossProduct(-CustomMovementComponent->GetCurrentClimbableSurfaceNormal(), -GetActorUpVector()).GetSafeNormal();

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}

}

void AClimbingSystemCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AClimbingSystemCharacter::Climbing(const FInputActionValue& Value)
{
	if (!CustomMovementComponent) return;

	if (CustomMovementComponent->IsClimbing())
	{
		CustomMovementComponent->ToggleClimbingMode(false);
	}
	else
	{
		CustomMovementComponent->ToggleClimbingMode(true);
	}
}
