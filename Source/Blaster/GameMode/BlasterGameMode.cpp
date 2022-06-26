// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		// 获得所有APlayerStart类的Actors，并存入PlayerStarts数组
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		// 随机选择重生点
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
 		
		// 通过BlasterPlayerController的OnPossess函数解决了重生后Health条的更新问题
// 		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ElimmedController->GetCharacter());
// 		if (BlasterCharacter)
// 		{
// 			BlasterCharacter->UpdateHUDHealth();
// 		}
// 		APlayerController* PlayerController = Cast<APlayerController>(ElimmedController);
// 		if (PlayerController)
// 		{
// 			InitializeHUDForPlayer(PlayerController);
// 		}
	}
}
