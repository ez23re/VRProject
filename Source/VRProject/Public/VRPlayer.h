#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/InputActionValue.h"
#include "VRPlayer.generated.h"

UCLASS()
class VRPROJECT_API AVRPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	AVRPlayer();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:

	UPROPERTY(VisibleAnywhere, Category="MotionController")
	class UMotionControllerComponent* LeftHand;
	
	UPROPERTY(VisibleAnywhere, Category="MotionController")
	class UMotionControllerComponent* RightHand;

	UPROPERTY(VisibleAnywhere, Category="MotionController")
	class UMotionControllerComponent* RightAim;

public:
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* VRCamera;

private:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputMappingContext* IMC_VRInput;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputAction* IA_VRMove;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputAction* IA_VRMouse;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	bool bUsingMouse = false;


	void Move(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);

public:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputAction* IA_VRTeleport;

	//bool bIsDebufDraw = false;
	//UFUNCTION(Exec)
	//void ActiveDebugDraw();

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* TeleportCircle;

	// 텔레포트 진행여부
	bool bTeleporting = false;

	// 텔레포트 리셋
	bool ResetTeleport();

	void TeleportStart(const FInputActionValue& Value);
	void TeleportEnd(const FInputActionValue& Value);

	// 직선 텔레포트 그리기
	void DrawTeleportStraight();

	// 텔레포트 위치
	FVector TeleportLocation;

public:
	// Curve Teleport
	// v = v0 + at랑 F = ma 사용함
	// 곡선을 이루는 점의 개수
	UPROPERTY(EditAnywhere, Category="Teleport")
	int32 LineSmooth=40;

	UPROPERTY(EditAnywhere, Category="Teleport")
	float CurveForce=1500.f;

	UPROPERTY(EditAnywhere, Category="Teleport")
	float Gravity=-5000.f;

	// Deltatime
	UPROPERTY(EditAnywhere, Category="Teleport")
	float SimulateTime=0.02f;

	// 기억할 점 리스트
	TArray<FVector> Lines;

	// 텔레포트 모드전환(Curve or not)
	UPROPERTY(EditAnywhere, Category="Teleport")
	bool bTeleportCurve = true;

	// 곡선 텔레포트 그리기
	void DrawTeleportCurve();

	bool CheckHitTeleport(FVector LastPos, FVector& CurPos);


public:
	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* TeleportUIComponent;

private:
	// Warp 활성화 여부
	UPROPERTY(EditAnywhere, Category="Warp", meta = (AllowPrivateAccess=true))
	bool IsWarp = true;
	
	// 1. 등속->정해진 시간에 맞춰 이동하기
	// 경과시간
	float CurrentTime = 0;
	
	// 워프타임
	UPROPERTY(EditAnywhere, Category="Warp", meta = (AllowPrivateAccess=true))
	float WarpTime = 0.2f;

	FTimerHandle WarpTimer;

	// 워프수행함수
	void DoWarp();

public: // 총쏘기
	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputAction* IA_Fire;
	void FireInput(const FInputActionValue& Value);


	// CrossHair
	UPROPERTY(VisibleAnywhere)
	class UChildActorComponent* CrossHairComp;

	void DrawCrossHair();

public: // 잡기
	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputAction* IA_Grab;

	// 물체를 잡고 있는지 여부
	bool bIsGrabbing = false;

	// 필요속성: 잡을 범위
	UPROPERTY(EditAnywhere, Category="Grab")
	float GrabRadius = 100;

	// 잡은 물체 기억할 변수 추가
	UPROPERTY()
	class UPrimitiveComponent* grabbedObject;

	void TryGrab(const FInputActionValue& Value);
	void TryUnGrb(const FInputActionValue& Value);
	// 물체를 잡은 상태로 컨트롤 하기
	void Grabbing();


	// 이전 위치
	FVector PrePos;
	// 이전 회전
	FQuat PreRot;
	// 던질 힘
	UPROPERTY(EditAnywhere, Category="Grab")
	float ThrowPower = 500;
	// 회전 힘
	UPROPERTY(EditAnywhere, Category="Grab")
	float ToquePower = 500;
	// 던질 방향
	FVector ThrowDirection;
	// 회전할 방향
	FQuat DeltaRotation;




};	