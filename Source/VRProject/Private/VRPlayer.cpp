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

	// 모션컨트롤러 컴포넌트 추가
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

	// 텔레포트 활성화시 처리
	if (bTeleporting) {
		// 텔레포트 그리기
		if (bTeleportCurve) {
			DrawTeleportCurve();
		}
		else {
			DrawTeleportStraight();
		}
	}

	// Niagara Curve가 보여지면 데이터 세팅하도록
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

		// 텔레포트
		InputSystem->BindAction(IA_VRTeleport, ETriggerEvent::Started, this, &AVRPlayer::TeleportStart);
		InputSystem->BindAction(IA_VRTeleport, ETriggerEvent::Completed, this, &AVRPlayer::TeleportEnd);

		// 총쏘기
		InputSystem->BindAction(IA_Fire, ETriggerEvent::Started, this, &AVRPlayer::FireInput);

		// 잡기
		InputSystem->BindAction(IA_Grab, ETriggerEvent::Started, this, &AVRPlayer::TryGrab);
		InputSystem->BindAction(IA_Grab, ETriggerEvent::Completed, this, &AVRPlayer::TryUnGrb);


	}

	/* 내 방법
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

	// 다른 방법 2가지
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

// 텔레포트 설정을 초기화
// 현재 텔레포트가 가능한지도 결과로 넘겨주자
bool AVRPlayer::ResetTeleport()
{
	// 현재 텔레포트Circle이 보여지고 있으면 이동가능, 그렇지 않으면 X
	bool bCanTeleport = TeleportCircle->GetVisibleFlag();
	
	// 텔레포트 종료
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
	// 텔레포트 가능하지 않다면 아무처리 X
	if (!ResetTeleport()) {
		return;
	}

	// 이동
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
	// 1. Line 만들기
	FVector StartPoint = VRCamera->GetComponentLocation();
	FVector EndPoint = StartPoint + RightHand->GetForwardVector() * 1000;
	
	// 2. Line 쏘기
	bool bHit = CheckHitTeleport(StartPoint, EndPoint);

	Lines.Empty();
	Lines.Add(StartPoint);
	Lines.Add(EndPoint);

	// 선 그리기
	//DrawDebugLine(GetWorld(), StartPoint, EndPoint, FColor::Red, false, -1, 0, 2);
}

void AVRPlayer::DrawTeleportCurve()
{
	Lines.Empty();

	// 선이 진행될 힘(방향)
	FVector Velocity = RightHand->GetForwardVector() * CurveForce;
	
	// 최초 위치 P0 
	FVector Pos = RightHand->GetComponentLocation();
	Lines.Add(Pos);
	
	// 40번 반복하겠다 
	for (int i = 0;i < LineSmooth;++i) {
		
		FVector LastPos = Pos;
		// v = v0 + at
		Velocity += FVector::UpVector * Gravity * SimulateTime;
		// P = P0 + vt
		Pos += Velocity * SimulateTime;
		
		bool bHit = CheckHitTeleport(LastPos, Pos);

		// 부딪혔을 때는 for문 중단
		Lines.Add(Pos);
		if (bHit) {
			break;
		}
	}

	// 선 그리기
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

	// <내가 직접 해보기> 부딪힌 오브젝트의 이름이 Floor라면 Circle 그리고, 아니라면 보여주지 말기
	// Floor 위치로만 가기 -> Z값=0인 곳
	/* FName Floor = HitInfo.GetActor()->GetActorNameOrLabel("Floor");
	   Floor->SetActorLocation((HitInfo.Location).Z); */

	// 3. Line과 충돌시
    // 4. 그리고 충돌한 녀석의 이름에 Floor가 있다면

	if (bHit && HitInfo.GetActor()->GetActorNameOrLabel().Contains("Floor") == true) {
		// 텔레포트 UI 활성화
		TeleportCircle->SetVisibility(true);
		// 부딪힌 지점에 텔레포트Circle 위치시키기
		TeleportCircle->SetWorldLocation(HitInfo.Location);
		// 텔레포트 위치 지정
		TeleportLocation = HitInfo.Location;

		CurPos = TeleportLocation;
	}
	// 4. 안 부딪혔으면 Circle NO
	else {
		TeleportCircle->SetVisibility(false);
	}

	return bHit;
}

void AVRPlayer::DoWarp()
{
	// 1. 워프 활성화 되어있을 때만 수행
	if (IsWarp == false) {
		return;
	}

	CurrentTime = 0.f;

	// 충돌체 꺼주자
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetWorld()->GetTimerManager().SetTimer(WarpTimer, FTimerDelegate::CreateLambda(
		[this]()->void
		{
			// From -> To로 Percent 이용해 이동한다
			// 만약 목적지에 거~의 도착하면 도착한 것으로 처리한다
			// 2-1. 시간이 흘러야
			CurrentTime += GetWorld()->DeltaTimeSeconds;
			// 2-2. Warp Time까지 이동하고 싶다
			FVector TargetPos = TeleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector CurPos = GetActorLocation();

			CurPos = FMath::Lerp(CurPos, TargetPos, 10 * GetWorld()->DeltaTimeSeconds);
			SetActorLocation(CurPos);

			// 목적지에 도착했다면, 목적지 위치로 보정, 타이머OFF, 충돌체ON
			// -> 일정 범위안에 들어왔다면 (구총돌, 원충돌)
			float Distance = FVector::Distance(CurPos, TargetPos);
			if (Distance<10) {
				SetActorLocation(TargetPos);
				GetWorld()->GetTimerManager().ClearTimer(WarpTimer);
				GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}

			//if (CurrentTime >= WarpTime) {  // 조건문 수정
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
		// 부딪힌 녀석이 물리 기능이 활성화 되어있으면 날려보내자
		auto HitComp = HitInfo.GetComponent();
		if (HitComp && HitComp->IsAnySimulatingPhysics()) {
			HitComp->AddImpulseAtLocation(RightAim->GetForwardVector() * 1000, HitInfo.Location);
			
		}
	}
}

void AVRPlayer::DrawCrossHair()
{
	// 오른손이 LineTrace 쏘도록
	FVector StartPos = RightAim->GetComponentLocation();
	FVector EndPos = StartPos + RightAim->GetForwardVector() * 10000;
	FHitResult HitInfo;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	DrawDebugLine(GetWorld(), StartPos, EndPos, FColor::Yellow);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitInfo, StartPos, EndPos, ECC_Visibility, Params);

	float Distance = 0;
	// 부딪혔을 경우
	if (bHit) {
		CrossHairComp->SetWorldLocation(HitInfo.Location);
		Distance = FVector::Distance(VRCamera->GetComponentLocation(), HitInfo.Location); // HitInfo.Distance 안쓰는 이유, 손을 기준으로 잡았기 때문!
	}
	else {
		CrossHairComp->SetWorldLocation(EndPos);
		Distance = FVector::Distance(VRCamera->GetComponentLocation(), EndPos); 

	}

	// 거리 값을 가지고 크기 설정을 해주겠다
	// 최솟값은 1, 최댓값은 위에서 구한다
	Distance = FMath::Max(1, Distance);
	
	// 크기 설정
	CrossHairComp->SetWorldScale3D(FVector(Distance));

	// 빌보딩 -> 카메라쪽으로 바라보도록 하자
	FVector Direction = CrossHairComp->GetComponentLocation() - VRCamera->GetComponentLocation();
	CrossHairComp->SetWorldRotation(Direction.Rotation());



}

// 일정 범위 안에 있는 물체를 잡고 싶다
// 필요속성: 잡을 범위
void AVRPlayer::TryGrab(const FInputActionValue& Value)
{
	// 이미 잡고 있을 때는 그만 잡게 해야지 -> UnGrab() 구현하면 됨

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FVector HandPos = RightHand->GetComponentLocation();
	TArray<FOverlapResult> HitObjects;

	// 다중 + 가장 가까운 거 체크
	bool bHit = GetWorld()->OverlapMultiByChannel(HitObjects, HandPos, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(GrabRadius), Params);
	
	// 충돌한 물체가 없으면 아무처리 X
	if (bHit == false) {
		return;
	}

	// 가장 가까운 물체를 검출하자 -> 단순 인덱스 검사만
	int Closest = -1;
	// 손과 가장 가까운 물체를 검색
	for (int i = 0;i < HitObjects.Num();++i) {
		// 물체를 던지고 싶다 (벽 같은 건 X) -> Physics가 활성화 되어 있는 물체만!
		auto HitComp = HitObjects[i].GetComponent();
		if (HitComp->IsSimulatingPhysics() == false) {
			continue;
		}

		if (Closest == -1) {
			Closest = i;
		}

		// 물체를 잡고 있는 상태로 설정
		bIsGrabbing = true; // i=1부터 하면 0번째도 가져와버릴 수도 있다!

		// 현재 가장 손과 가까운 위치
		FVector ClosestPos = HitObjects[Closest].GetActor()->GetActorLocation();
		float ClosesetDistacne = FVector::Distance(ClosestPos, HandPos);

		// 다음 물체와 손과의 거리
		FVector NextPos = HitObjects[i].GetActor()->GetActorLocation();
		float Nextdistance = FVector::Distance(NextPos, HandPos);

		// 다음 물체가 손과 더 가까우면 갱신해주기
		if (Nextdistance < ClosesetDistacne) {
			// 가장 가까운 물체 인덱스 교체
			Closest = i;
		}
	}

	//// HitObjects 중에서 내 RightHand랑 거리가 가장 가까운 물체 가져오기
	//// 1. HitObjects 중에서 특정 오브젝트 가져오기
	//for (const FOverlapResult& Hit : HitObjects) {
	//	AActor* HitActor = Hit.GetActor();
	//	if (HitActor) {
	//		// 2. RightHand.Distance랑 Min() 가장 가까운 물체 찾기

	//	}
	//}

	// for문 끝나고! 물체를 잡았다면 손에 붙여주자
	if (bIsGrabbing) {
		grabbedObject = HitObjects[Closest].GetComponent();
		// 붙이기 전에 물리 OFF, 충돌체도 꺼주자
		grabbedObject->SetSimulatePhysics(false);
		grabbedObject->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HitObjects[Closest].GetActor()->SetActorLocation(HandPos);

		grabbedObject->AttachToComponent(RightHand, FAttachmentTransformRules::KeepWorldTransform);
		
		// 초기값 설정
		PrePos = RightHand->GetComponentLocation();
		PreRot == RightHand->GetComponentQuat();
		
		UE_LOG(LogTemp, Warning, TEXT("Grab!!!!!!!!!!!!!!!!!!!"));
	}

}

void AVRPlayer::TryUnGrb(const FInputActionValue& Value)
{
	// 물체를 잡고 있지 않다면
	if (bIsGrabbing == false) {
		return;
	}
	
	// 손에 붙어있는 물체를 Detach
	grabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	bIsGrabbing = false;

	// 물리기능, 충돌체 활성화
	grabbedObject->SetSimulatePhysics(true);
	grabbedObject->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 던지기
	grabbedObject->AddImpulse(ThrowDirection * ThrowPower, NAME_None, true);

	grabbedObject = nullptr;

	
}

// 던질 방향, 회전값 구하기
// 이전위치 -> 현재위치 차이를 이용해 방향 구하기
void AVRPlayer::Grabbing()
{
	// 잡은게 없으면 return
	if (bIsGrabbing == false) {
		return;
	}

	// 던질방향
	ThrowDirection = RightHand->GetComponentLocation() - PrePos;
	// 노멀라이즈 해줘야돼? 뒤에서 쏘면 나한테 와야지 그럴필요X

	// 회전변화량 구하기
	/* 쿼터니언 공식
	* angle1 = Q1, angle = Q2
	* angle1 + angle2 = Q1 * Q2
	* -angle2 = Q2.Inverse()
	* angle2 - angle1 = Q2 * Q1.Inverse()
	*/
	DeltaRotation = RightHand->GetComponentQuat() * PreRot.Inverse();

	PrePos = RightHand->GetComponentLocation();
	PreRot = RightHand->GetComponentQuat();


}
