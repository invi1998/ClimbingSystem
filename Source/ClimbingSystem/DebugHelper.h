#pragma once

namespace CS_Debug
{
	static void Print(const FString& Message, const FColor& InColor = FColor::MakeRandomColor(), int32 InKey = INDEX_NONE, float InTimeToDisplay = 5.0f)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(InKey, InTimeToDisplay, InColor, Message);
		}

		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	}
}
