// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);			// ����HUD��
	void SetHUDAnnouncementCountdown(float CountdownTime);		// ����HUD��
	void SetHUDGrenades(int32 Grenades);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); // Synced with server world clock
	virtual void ReceivedPlayer() override; // Sync with server clock as soon as possible

	void OnMatchStateSet(FName State);

	void HandleMatchHasStarted();
	void HandleCooldown();

protected:
	virtual void BeginPlay() override;

	void SetHUDTime();			// ����HUD����Ƶ�ʷ�ʽ

	void PollInit();
	/**
	* Sync time between client and server
	*/

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f;			// difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;			// ͬ��Ƶ�ʣ�Ĭ��5��ͬ��һ�Σ�������ͼ���޸ģ�

	float TimeSyncRunningTime = 0.f;		// ��¼ÿ��ͬ�������е�ʱ��
	void CheckTimeSync(float DeltaTime);	// ͬ������

	// Set time
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterMode;

	float MatchTime = 0.f;					// ��GameMode��ȡ
	float WarmupTime = 0.f;					// ��GameMode��ȡ
	float CooldownTime = 0.f;					// ��GameMode��ȡ
	float LevelStartingTime = 0.f;			// ��GameMode��ȡ

	uint32 CountdownInt = 0;		// ����������������Ÿ���HUD

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	float HUDHealth;
	bool bInitializeHealth = false;
	float HUDShield;
	float HUDMaxShield;
	bool bInitializeShield = false;
	float HUDMaxHealth;
	float HUDScore;
	bool bInitializeScore = false;
	int32 HUDDefeats;
	bool bInitializeDefeats = false;
	int32 HUDGrenades;
	bool bInitializeGrenades = false;

};
