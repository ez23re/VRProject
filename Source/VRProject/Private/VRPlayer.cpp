#include "VRPlayer.h"
#include "Camera/CameraComponent.h"
#include "../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/InputMappingContext.h"
#include "../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/InputAction.h"
#include "../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputComponent.h"
#include "../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "MotionControllerComponent.h"
#include "../../../../Plugins/FX/Niagara/Source/Niagara/Public/NiagaraComponent.h"
#include "../../../../Plugins/FX/Niagara/Source/Niagara/Classes/NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "../standalone_prologue.h"
#include "Engine/TimerHandle.h"
#include "Components/ChildActorComponent.h"

AVRPlayer::AVRPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	VRCamera = CreateDefaultSubobject<UCameraComponent>("VRCamera");
	VRCamera->SetupAttachment(RootComponent);

	// �����Ʈ�ѷ� ������Ʈ �߰�
	LeftHand = CreateDefaultSubobject<UMotionControllerComponent>("LeftHand");
	LeftHand->SetupAttachment(RootComponent);
	LeftHand->SetTrackingMotionSource(TEXT("Left"));

	RightHand = CreateDefaultSubobject<UMotionControllerComponent>("RightHand");
	RightHand->SetupAttachment(RootComponent);
	RightHand->SetTrackingMotionSource(TEXT("Right"));

	// Aim
	RightAim = CreateDefaultSubobject<UMotionControllerComponent>("RightAim");
	RightAim->SetupAttachment(RootComponent);
	RightAim->SetTrackingMotionSource(TEXT("RightAim"));
	

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_VRINPUT(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/LSJ/Inputs/IMC_VRInput.IMC_VRInput'"));
	if (IMC_VRINPUT.Succeeded()) {
		IMC_VRInput = IMC_VRINPUT.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_VRMOVE(TEXT("/Script/EnhancedInput.InputAction'/Game/LSJ/Inputs/IA_VRMove.IA_VRMove'"));
	if (IA_VRMOVE.Succeeded()) {
		IA_VRMove = IA_VRMOVE.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_VRMOUSE(TEXT("/Script/EnhancedInput.InputAction'/Game/LSJ/Inputs/IA_VRMouse.IA_VRMouse'"));
	if (IA_VRMOUSE.Succeeded()) {
		IA_VRMouse = IA_VRMOUSE.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_VRTELEPORT(TEXT("/Script/EnhancedInput.InputAction'/Game/LSJ/Inputs/IA_VRTeleport.IA_VRTeleport'"));
	if (IA_VRTELEPORT.Succeeded()) {
		IA_VRTeleport = IA_VRTELEPORT.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_FIRE(TEXT("/Script/EnhancedInput.InputAction'/Game/LSJ/Inputs/IA_Fire.IA_Fire'"));
	if (IA_FIRE.Succeeded()) {
		IA_Fire = IA_FIRE.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_GRAB(TEXT("/Script/EnhancedInput.InputAction'/Game/LSJ/Inputs/IA_Grab.IA_Grab'"));
	if (IA_GRAB.Succeeded()) {
		IA_Grab = IA_GRAB.Object;
	}

	TeleportCircle = CreateDefaultSubobject<UNiagaraComponent>("TeleportCircle");
	TeleportCircle->SetupAttachment(RootComponent);

	// Teleport UI
	TeleportUIComponent = CreateDefaultSubobject<UNiagaraComponent>("TeleportUIComponent");
	TeleportUIComponent->SetupAttachment(RootComponent);
	
	// CrossHair Component
	CrossHairComp = CreateDefaultSubobject<UChildActorComponent>("CrossHairComp");
	CrossHairComp->SetupAttachment(RootComponent);
	static ConstructorHelpers::FClassFinder<AActor> TempCrossHair(TEXT("'/Game/LSJ/Blueprints/BP_CrossHair.BP_CrossHair_C'"));
	if (TempCrossHair.Succeeded()) {
		CrossHairComp->SetChildActorClass(TempCrossHair.Class);
	}




}

void AVRPlayer::BeginPlay()
{
	Super::BeginPlay();

	ResetTeleport();
}

void AVRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawCrossHair();

	// �ڷ���Ʈ Ȱ��ȭ�� ó��
	if (bTeleporting) {
		// �ڷ���Ʈ �׸���
		if (bTeleportCurve) {
			DrawTeleportCurve();
		}
		else {
			DrawTeleportStraight();
		}
	}

	// Niagara Curve�� �������� ������ �����ϵ���
	// UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TeleportUIComponent, TEXT("User.PointArray"), Lines);
	


}

void AVRPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto PC = GetWorld()->GetFirstPlayerController();
	if (PC) {
		auto LocalPlayer = PC->GetLocalPlayer();
		auto SS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
		if (SS) {
			SS->AddMappingContext(IMC_VRInput, 1);
		}
	}
	auto InputSystem = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (InputSystem) {
		InputSystem->BindAction(IA_VRMove, ETriggerEvent::Triggered, this, &AVRPlayer::Move);
		InputSystem->BindAction(IA_VRMouse, ETriggerEvent::Triggered, this, &AVRPlayer::Turn);

		// �ڷ���Ʈ
		InputSystem->BindAction(IA_VRTeleport, ETriggerEvent::Started, this, &AVRPlayer::TeleportStart);
		InputSystem->BindAction(IA_VRTeleport, ETriggerEvent::Completed, this, &AVRPlayer::TeleportEnd);

		// �ѽ��
		InputSystem->BindAction(IA_Fire, ETriggerEvent::Started, this, &AVRPlayer::FireInput);

		// ���
		InputSystem->BindAction(IA_Grab, ETriggerEvent::Started, this, &AVRPlayer::TryGrab);
		InputSystem->BindAction(IA_Grab, ETriggerEvent::Completed, this, &AVRPlayer::TryUnGrb);


	}

	/* �� ���
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(IA_VRMove, ETriggerEvent::Triggered, this,  &AVRPlayer::Move);
	}
	*/
}

void AVRPlayer::Move(const FInputActionValue& Value)
{
	FVector2D Scale = Value.Get<FVector2D>();

	AddMovementInput(VRCamera->GetForwardVector(), Scale.X);
	AddMovementInput(VRCamera->GetRightVector(), Scale.Y);

	// �ٸ� ��� 2����
	//FVector Direction = VRCamera->GetForwardVector() * Scale.X + VRCamera->GetRightVector() * Scale.Y;
	
	/*
	FVector Direction = FVector(Scale.X, Scale.Y, 0);
	VRCamera->GetComponentTransform().TransformVector(Direction);
	*/
	
	//AddMovementInput(Direction);

}

void AVRPlayer::Turn(const FInputActionValue& Value)
{
	if (bUsingMouse==false) {
		return;
	}

	FVector2D Scale = Value.Get<FVector2D>();
	AddControllerPitchInput(Scale.Y);
	AddControllerYawInput(Scale.X);
}

// �ڷ���Ʈ ������ �ʱ�ȭ
// ���� �ڷ���Ʈ�� ���������� ����� �Ѱ�����
bool AVRPlayer::ResetTeleport()
{
	// ���� �ڷ���ƮCircle�� �������� ������ �̵�����, �׷��� ������ X
	bool bCanTeleport = TeleportCircle->GetVisibleFlag();
	
	// �ڷ���Ʈ ����
	bTeleporting = false;
	TeleportCircle->SetVisibility(false);
	TeleportUIComponent->SetVisibility(false);
	
	return bCanTeleport;
}

void AVRPlayer::TeleportStart(const FInputActionValue& Value)
{
	bTeleporting = true;
	TeleportUIComponent->SetVisibility(true);

}

void AVRPlayer::TeleportEnd(const FInputActionValue& Value)
{
	// �ڷ���Ʈ �������� �ʴٸ� �ƹ�ó�� X
	if (!ResetTeleport()) {
		return;
	}

	// �̵�
	if (IsWarp) {
		DoWarp();
	}
	else {
		SetActorLocation(TeleportLocation);
	}
}

void AVRPlayer::DrawTeleportStraight()
{
	// LineTrace
	// 1. Line �����
	FVector StartPoint = VRCamera->GetComponentLocation();
	FVector EndPoint = StartPoint + RightHand->GetForwardVector() * 1000;
	
	// 2. Line ���
	bool bHit = CheckHitTeleport(StartPoint, EndPoint);

	Lines.Empty();
	Lines.Add(StartPoint);
	Lines.Add(EndPoint);

	// �� �׸���
	//DrawDebugLine(GetWorld(), StartPoint, EndPoint, FColor::Red, false, -1, 0, 2);
}

void AVRPlayer::DrawTeleportCurve()
{
	Lines.Empty();

	// ���� ����� ��(����)
	FVector Velocity = RightHand->GetForwardVector() * CurveForce;
	
	// ���� ��ġ P0 
	FVector Pos = RightHand->GetComponentLocation();
	Lines.Add(Pos);
	
	// 40�� �ݺ��ϰڴ� 
	for (int i = 0;i < LineSmooth;++i) {
		
		FVector LastPos = Pos;
		// v = v0 + at
		Velocity += FVector::UpVector * Gravity * SimulateTime;
		// P = P0 + vt
		Pos += Velocity * SimulateTime;
		
		bool bHit = CheckHitTeleport(LastPos, Pos);

		// �ε����� ���� for�� �ߴ�
		Lines.Add(Pos);
		if (bHit) {
			break;
		}
	}

	// �� �׸���
	//int32 LineCount = Lines.Num();
	//for (int i = 0;i < LineCount-1;++i) {
	//	DrawDebugLine(GetWorld(), Lines[i], Lines[i + 1], FColor::Red, false, -1, 0, 1);
	//}


}

bool AVRPlayer::CheckHitTeleport(FVector LastPos, FVector& CurPos)
{
	TArray<AActor*> ignoreActor;
	FHitResult HitInfo;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitInfo, LastPos, CurPos, ECC_Visibility, Params);

	// <���� ���� �غ���> �ε��� ������Ʈ�� �̸��� Floor��� Circle �׸���, �ƴ϶�� �������� ����
	// Floor ��ġ�θ� ���� -> Z��=0�� ��
	/* FName Floor = HitInfo.GetActor()->GetActorNameOrLabel("Floor");
	   Floor->SetActorLocation((HitInfo.Location).Z); */

	// 3. Line�� �浹��
    // 4. �׸��� �浹�� �༮�� �̸��� Floor�� �ִٸ�

	if (bHit && HitInfo.GetActor()->GetActorNameOrLabel().Contains("Floor") == true) {
		// �ڷ���Ʈ UI Ȱ��ȭ
		TeleportCircle->SetVisibility(true);
		// �ε��� ������ �ڷ���ƮCircle ��ġ��Ű��
		TeleportCircle->SetWorldLocation(HitInfo.Location);
		// �ڷ���Ʈ ��ġ ����
		TeleportLocation = HitInfo.Location;

		CurPos = TeleportLocation;
	}
	// 4. �� �ε������� Circle NO
	else {
		TeleportCircle->SetVisibility(false);
	}

	return bHit;
}

void AVRPlayer::DoWarp()
{
	// 1. ���� Ȱ��ȭ �Ǿ����� ���� ����
	if (IsWarp == false) {
		return;
	}

	CurrentTime = 0.f;

	// �浹ü ������
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetWorld()->GetTimerManager().SetTimer(WarpTimer, FTimerDelegate::CreateLambda(
		[this]()->void
		{
			// From -> To�� Percent �̿��� �̵��Ѵ�
			// ���� �������� ��~�� �����ϸ� ������ ������ ó���Ѵ�
			// 2-1. �ð��� �귯��
			CurrentTime += GetWorld()->DeltaTimeSeconds;
			// 2-2. Warp Time���� �̵��ϰ� �ʹ�
			FVector TargetPos = TeleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector CurPos = GetActorLocation();

			CurPos = FMath::Lerp(CurPos, TargetPos, 10 * GetWorld()->DeltaTimeSeconds);
			SetActorLocation(CurPos);

			// �������� �����ߴٸ�, ������ ��ġ�� ����, Ÿ�̸�OFF, �浹üON
			// -> ���� �����ȿ� ���Դٸ� (���ѵ�, ���浹)
			float Distance = FVector::Distance(CurPos, TargetPos);
			if (Distance<10) {
				SetActorLocation(TargetPos);
				GetWorld()->GetTimerManager().ClearTimer(WarpTimer);
				GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}

			//if (CurrentTime >= WarpTime) {  // ���ǹ� ����
			//	SetActorLocation(TargetPos);
			//	GetWorld()->GetTimerManager().ClearTimer(WarpTimer);
			//	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			//}
		}
	), 0.02f, true);
}

void AVRPlayer::FireInput(const FInputActionValue& Value)
{
	FVector StartPos = RightAim->GetComponentLocation();
	FVector EndPos = StartPos + RightAim->GetForwardVector() * 10000;
	FHitResult HitInfo;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitInfo, StartPos, EndPos, ECC_Visibility, Params);
	if (bHit) {
		// �ε��� �༮�� ���� ����� Ȱ��ȭ �Ǿ������� ����������
		auto HitComp = HitInfo.GetComponent();
		if (HitComp && HitComp->IsAnySimulatingPhysics()) {
			HitComp->AddImpulseAtLocation(RightAim->GetForwardVector() * 1000, HitInfo.Location);
			
		}
	}
}

void AVRPlayer::DrawCrossHair()
{
	// �������� LineTrace ���
	FVector StartPos = RightAim->GetComponentLocation();
	FVector EndPos = StartPos + RightAim->GetForwardVector() * 10000;
	FHitResult HitInfo;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	DrawDebugLine(GetWorld(), StartPos, EndPos, FColor::Yellow);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitInfo, StartPos, EndPos, ECC_Visibility, Params);

	float Distance = 0;
	// �ε����� ���
	if (bHit) {
		CrossHairComp->SetWorldLocation(HitInfo.Location);
		Distance = FVector::Distance(VRCamera->GetComponentLocation(), HitInfo.Location); // HitInfo.Distance �Ⱦ��� ����, ���� �������� ��ұ� ����!
	}
	else {
		CrossHairComp->SetWorldLocation(EndPos);
		Distance = FVector::Distance(VRCamera->GetComponentLocation(), EndPos); 

	}

	// �Ÿ� ���� ������ ũ�� ������ ���ְڴ�
	// �ּڰ��� 1, �ִ��� ������ ���Ѵ�
	Distance = FMath::Max(1, Distance);
	
	// ũ�� ����
	CrossHairComp->SetWorldScale3D(FVector(Distance));

	// ������ -> ī�޶������� �ٶ󺸵��� ����
	FVector Direction = CrossHairComp->GetComponentLocation() - VRCamera->GetComponentLocation();
	CrossHairComp->SetWorldRotation(Direction.Rotation());



}

// ���� ���� �ȿ� �ִ� ��ü�� ��� �ʹ�
// �ʿ�Ӽ�: ���� ����
void AVRPlayer::TryGrab(const FInputActionValue& Value)
{
	// �̹� ��� ���� ���� �׸� ��� �ؾ��� -> UnGrab() �����ϸ� ��

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FVector HandPos = RightHand->GetComponentLocation();
	TArray<FOverlapResult> HitObjects;

	// ���� + ���� ����� �� üũ
	bool bHit = GetWorld()->OverlapMultiByChannel(HitObjects, HandPos, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(GrabRadius), Params);
	
	// �浹�� ��ü�� ������ �ƹ�ó�� X
	if (bHit == false) {
		return;
	}

	// ���� ����� ��ü�� �������� -> �ܼ� �ε��� �˻縸
	int Closest = -1;
	// �հ� ���� ����� ��ü�� �˻�
	for (int i = 0;i < HitObjects.Num();++i) {
		// ��ü�� ������ �ʹ� (�� ���� �� X) -> Physics�� Ȱ��ȭ �Ǿ� �ִ� ��ü��!
		auto HitComp = HitObjects[i].GetComponent();
		if (HitComp->IsSimulatingPhysics() == false) {
			continue;
		}

		if (Closest == -1) {
			Closest = i;
		}

		// ��ü�� ��� �ִ� ���·� ����
		bIsGrabbing = true; // i=1���� �ϸ� 0��°�� �����͹��� ���� �ִ�!

		// ���� ���� �հ� ����� ��ġ
		FVector ClosestPos = HitObjects[Closest].GetActor()->GetActorLocation();
		float ClosesetDistacne = FVector::Distance(ClosestPos, HandPos);

		// ���� ��ü�� �հ��� �Ÿ�
		FVector NextPos = HitObjects[i].GetActor()->GetActorLocation();
		float Nextdistance = FVector::Distance(NextPos, HandPos);

		// ���� ��ü�� �հ� �� ������ �������ֱ�
		if (Nextdistance < ClosesetDistacne) {
			// ���� ����� ��ü �ε��� ��ü
			Closest = i;
		}
	}

	//// HitObjects �߿��� �� RightHand�� �Ÿ��� ���� ����� ��ü ��������
	//// 1. HitObjects �߿��� Ư�� ������Ʈ ��������
	//for (const FOverlapResult& Hit : HitObjects) {
	//	AActor* HitActor = Hit.GetActor();
	//	if (HitActor) {
	//		// 2. RightHand.Distance�� Min() ���� ����� ��ü ã��

	//	}
	//}

	// for�� ������! ��ü�� ��Ҵٸ� �տ� �ٿ�����
	if (bIsGrabbing) {
		grabbedObject = HitObjects[Closest].GetComponent();
		// ���̱� ���� ���� OFF, �浹ü�� ������
		grabbedObject->SetSimulatePhysics(false);
		grabbedObject->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HitObjects[Closest].GetActor()->SetActorLocation(HandPos);

		grabbedObject->AttachToComponent(RightHand, FAttachmentTransformRules::KeepWorldTransform);
		
		// �ʱⰪ ����
		PrePos = RightHand->GetComponentLocation();
		PreRot == RightHand->GetComponentQuat();
		
		UE_LOG(LogTemp, Warning, TEXT("Grab!!!!!!!!!!!!!!!!!!!"));
	}

}

void AVRPlayer::TryUnGrb(const FInputActionValue& Value)
{
	// ��ü�� ��� ���� �ʴٸ�
	if (bIsGrabbing == false) {
		return;
	}
	
	// �տ� �پ��ִ� ��ü�� Detach
	grabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	bIsGrabbing = false;

	// �������, �浹ü Ȱ��ȭ
	grabbedObject->SetSimulatePhysics(true);
	grabbedObject->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// ������
	grabbedObject->AddImpulse(ThrowDirection * ThrowPower, NAME_None, true);

	grabbedObject = nullptr;

	
}

// ���� ����, ȸ���� ���ϱ�
// ������ġ -> ������ġ ���̸� �̿��� ���� ���ϱ�
void AVRPlayer::Grabbing()
{
	// ������ ������ return
	if (bIsGrabbing == false) {
		return;
	}

	// ��������
	ThrowDirection = RightHand->GetComponentLocation() - PrePos;
	// ��ֶ����� ����ߵ�? �ڿ��� ��� ������ �;��� �׷��ʿ�X

	// ȸ����ȭ�� ���ϱ�
	/* ���ʹϾ� ����
	* angle1 = Q1, angle = Q2
	* angle1 + angle2 = Q1 * Q2
	* -angle2 = Q2.Inverse()
	* angle2 - angle1 = Q2 * Q1.Inverse()
	*/
	DeltaRotation = RightHand->GetComponentQuat() * PreRot.Inverse();

	PrePos = RightHand->GetComponentLocation();
	PreRot = RightHand->GetComponentQuat();


}
