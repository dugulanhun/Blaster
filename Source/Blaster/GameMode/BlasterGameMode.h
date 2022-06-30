// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown;	// Match duration has been reached. Display winner and begin cooldown timer.
}

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;

	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;		// 进入游戏前的热身时间长度

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;		// 游戏时间长度

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;		// 游戏结算时间长度

	float LevelStartingTime = 0.f;	// 记录关卡加载时间，并从热身时间中剔除

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;		// 热身倒计时

public:
	FORCEINLINE float GetCountDownTime() { return CountdownTime; }
};
