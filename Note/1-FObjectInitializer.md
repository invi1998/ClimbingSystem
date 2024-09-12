# FObjectInitializer

`FObjectInitializer` 是 Unreal Engine (UE) 中的一个内部类，它主要用于对象初始化过程中的各种初始化任务。在 Unreal Engine 4 中，`FObjectInitializer` 被广泛用于在构造函数之外执行一些额外的初始化逻辑，尤其是在涉及到依赖注入和子对象初始化的情况下。虽然您提到的是 UE5，但请理解`FObjectInitializer`的概念和用法在UE4和UE5中应该是相似的，因为UE5继承和发展了UE4的许多核心机制。

`FObjectInitializer` 主要有以下几个用途：

1. **子对象初始化**：允许你在对象构造之后立即初始化任何子对象。
2. **依赖注入**：可以在构造期间注入依赖项，比如接口实现或其他对象实例。
3. **延迟加载**：对于某些资源或对象，可以延迟其加载直到真正需要时才加载。

### 如何使用 `FObjectInitializer`

在 Unreal Engine 中，`FObjectInitializer` 通常是在对象的构造函数中被调用的。下面是一个简单的示例，展示如何使用 `FObjectInitializer` 来初始化一个 `UObject` 的子类：

```cpp
UClass* UMyObject::StaticClass()
{
    static const UClass* Clss = FindObject<UFunction>(ANY_PACKAGE, TEXT("Class MyPlugin.MyObject"));
    return Clss;
}

UMyObject::UMyObject(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 初始化子对象
    SubObject = CreateDefaultSubobject<USomeComponent>(TEXT("SubObjectName"));
}
```

在这个例子中，`UMyObject` 是一个继承自 `UObject` 的类，它接受一个 `FObjectInitializer` 类型的参数。通过这个构造函数，你可以创建默认子对象，并进行其他初始化操作。



# 自定义角色移动组件

这里我们就需要使用FObjectInitializer来在Character的构造函数里初始化替换为我们自定义的移动组件

```c++
UCLASS(config=Game)
class AClimbingSystemCharacter : public ACharacter
{
	GENERATED_BODY()


public:
	AClimbingSystemCharacter(const FObjectInitializer& ObjectInitializer);
```

然后在Cpp里

```c++
//////////////////////////////////////////////////////////////////////////
// AClimbingSystemCharacter

AClimbingSystemCharacter::AClimbingSystemCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
```

