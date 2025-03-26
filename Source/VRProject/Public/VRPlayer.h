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

	// �ڷ���Ʈ ���࿩��
	bool bTeleporting = false;

	// �ڷ���Ʈ ����
	bool ResetTeleport();

	void TeleportStart(const FInputActionValue& Value);
	void TeleportEnd(const FInputActionValue& Value);

	// ���� �ڷ���Ʈ �׸���
	void DrawTeleportStraight();

	// �ڷ���Ʈ ��ġ
	FVector TeleportLocation;

public:
	// Curve Teleport
	// v = v0 + at�� F = ma �����
	// ��� �̷�� ���� ����
	UPROPERTY(EditAnywhere, Category="Teleport")
	int32 LineSmooth=40;

	UPROPERTY(EditAnywhere, Category="Teleport")
	float CurveForce=1500.f;

	UPROPERTY(EditAnywhere, Category="Teleport")
	float Gravity=-5000.f;

	// Deltatime
	UPROPERTY(EditAnywhere, Category="Teleport")
	float SimulateTime=0.02f;

	// ����� �� ����Ʈ
	TArray<FVector> Lines;

	// �ڷ���Ʈ �����ȯ(Curve or not)
	UPROPERTY(EditAnywhere, Category="Teleport")
	bool bTeleportCurve = true;

	// � �ڷ���Ʈ �׸���
	void DrawTeleportCurve();

	bool CheckHitTeleport(FVector LastPos, FVector& CurPos);


public:
	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* TeleportUIComponent;

private:
	// Warp Ȱ��ȭ ����
	UPROPERTY(EditAnywhere, Category="Warp", meta = (AllowPrivateAccess=true))
	bool IsWarp = true;
	
	// 1. ���->������ �ð��� ���� �̵��ϱ�
	// ����ð�
	float CurrentTime = 0;
	
	// ����Ÿ��
	UPROPERTY(EditAnywhere, Category="Warp", meta = (AllowPrivateAccess=true))
	float WarpTime = 0.2f;

	FTimerHandle WarpTimer;

	// ���������Լ�
	void DoWarp();

public: // �ѽ��
	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputAction* IA_Fire;
	void FireInput(const FInputActionValue& Value);


	// CrossHair
	UPROPERTY(VisibleAnywhere)
	class UChildActorComponent* CrossHairComp;

	void DrawCrossHair();

public: // ���
	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputAction* IA_Grab;

	// ��ü�� ��� �ִ��� ����
	bool bIsGrabbing = false;

	// �ʿ�Ӽ�: ���� ����
	UPROPERTY(EditAnywhere, Category="Grab")
	float GrabRadius = 100;

	// ���� ��ü ����� ���� �߰�
	UPROPERTY()
	class UPrimitiveComponent* grabbedObject;

	void TryGrab(const FInputActionValue& Value);
	void TryUnGrb(const FInputActionValue& Value);
	// ��ü�� ���� ���·� ��Ʈ�� �ϱ�
	void Grabbing();


	// ���� ��ġ
	FVector PrePos;
	// ���� ȸ��
	FQuat PreRot;
	// ���� ��
	UPROPERTY(EditAnywhere, Category="Grab")
	float ThrowPower = 500;
	// ȸ�� ��
	UPROPERTY(EditAnywhere, Category="Grab")
	float ToquePower = 500;
	// ���� ����
	FVector ThrowDirection;
	// ȸ���� ����
	FQuat DeltaRotation;




};	