// /****************************************************************************** * Copyright (C) Ultraleap, Ltd. 2011-2021. * *
// * * Use subject to the terms of the Apache License 2.0 available at            * * http://www.apache.org/licenses/LICENSE-2.0, or
// another agreement           * * between Ultraleap and you, your company or other organization.             *
// ******************************************************************************/

#include "OpenXRToLeapWrapper.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "IHandTracker.h"
#include "HeadMountedDisplayTypes.h"
#include "LeapUtility.h"

FOpenXRToLeapWrapper::FOpenXRToLeapWrapper() : HandTracker(nullptr)
{

	CurrentDeviceInfo = &DummyDeviceInfo;
	DummyDeviceInfo = {0};
	DummyLeapHands[0] = {0};
	DummyLeapHands[1] = {0};
	
	DummyLeapHands[0].type = eLeapHandType::eLeapHandType_Left;
	DummyLeapHands[1].type = eLeapHandType::eLeapHandType_Right;

	DummyLeapFrame = {0};

	DummyLeapFrame.framerate = 90;
	DummyLeapFrame.pHands = DummyLeapHands;

	
}

FOpenXRToLeapWrapper::~FOpenXRToLeapWrapper()
{
}
LEAP_CONNECTION* FOpenXRToLeapWrapper::OpenConnection(LeapWrapperCallbackInterface* InCallbackDelegate)
{
	CallbackDelegate = InCallbackDelegate;
	InitOpenXRHandTrackingModule();
	return nullptr;
}

void FOpenXRToLeapWrapper::InitOpenXRHandTrackingModule()
{
	IModuleInterface* ModuleInterface = FModuleManager::Get().LoadModule("OpenXRHandTracking");
	IModularFeatures& ModularFeatures = IModularFeatures::Get();
	if (ModularFeatures.IsModularFeatureAvailable(IHandTracker::GetModularFeatureName()))
	{
		HandTracker = &IModularFeatures::Get().GetModularFeature<IHandTracker>(IHandTracker::GetModularFeatureName());
		bIsConnected = true;

		if (CallbackDelegate)
		{
			CallbackDelegate->OnDeviceFound(&DummyDeviceInfo);
		}
	}
}
LEAP_VECTOR ConvertPositionToLeap(const FVector& FromOpenXR)
{
	LEAP_VECTOR Ret = FLeapUtility::ConvertAndScaleUEToLeap(FromOpenXR);	
	return Ret;
}
LEAP_QUATERNION ConvertOrientationToLeap(const FQuat& FromOpenXR)
{
	LEAP_QUATERNION Ret = {0};
		
	Ret.x = -FromOpenXR.Y;
	Ret.y = FromOpenXR.X;
	Ret.z = FromOpenXR.Z;
	Ret.w = FromOpenXR.W;

	return Ret;
}
// FOccluderVertexArray is really an array of vectors, don't know why this type was used in UE
void FOpenXRToLeapWrapper::ConvertToLeapSpace(LEAP_HAND& LeapHand, const FOccluderVertexArray& Positions,const TArray<FQuat>& Rotations)
{
	
	// Enums for each bone are 
	uint8 KeyPoint = 0;
	for (auto Position : Positions)
	{
		auto Rotation = Rotations[KeyPoint];

		EHandKeypoint eKeyPoint = (EHandKeypoint) KeyPoint;
		switch (eKeyPoint)
		{
			case EHandKeypoint::Palm:
				// wrist orientation comes from palm orientation in bodystate
				LeapHand.palm.orientation = ConvertOrientationToLeap(Rotation);
				LeapHand.palm.position = ConvertPositionToLeap(Position);
				break;
			case EHandKeypoint::Wrist:
				// wrist comes from arm next joint in bodystate
				LeapHand.arm.next_joint = ConvertPositionToLeap(Position);
				// set arm rotation from Wrist
				LeapHand.arm.rotation = ConvertOrientationToLeap(Rotation);
				break;
				// Thumb ////////////////////////////////////////////////////
			case EHandKeypoint::ThumbMetacarpal:
				LeapHand.thumb.metacarpal.prev_joint = ConvertPositionToLeap(Position);
				LeapHand.thumb.metacarpal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::ThumbProximal:
				LeapHand.thumb.proximal.prev_joint = LeapHand.thumb.metacarpal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.thumb.proximal.rotation = ConvertOrientationToLeap(Rotation);
				break;
			case EHandKeypoint::ThumbDistal:
				LeapHand.thumb.distal.prev_joint = LeapHand.thumb.proximal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.thumb.distal.rotation = ConvertOrientationToLeap(Rotation);
				break;
			case EHandKeypoint::ThumbTip:
				// tip is next of distal?
				LeapHand.thumb.distal.next_joint = ConvertPositionToLeap(Position);	
				break;

			// Index ////////////////////////////////////////////////////
			case EHandKeypoint::IndexMetacarpal:
				LeapHand.index.metacarpal.prev_joint = ConvertPositionToLeap(Position);
				LeapHand.index.metacarpal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::IndexProximal:
				LeapHand.index.proximal.prev_joint = LeapHand.index.metacarpal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.index.proximal.rotation = ConvertOrientationToLeap(Rotation);
		
				break;
			case EHandKeypoint::IndexIntermediate:
				LeapHand.index.intermediate.prev_joint = LeapHand.index.proximal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.index.intermediate.rotation = ConvertOrientationToLeap(Rotation);
		
				break;
			case EHandKeypoint::IndexDistal:
				LeapHand.index.distal.prev_joint = LeapHand.index.intermediate.next_joint = ConvertPositionToLeap(Position);
				LeapHand.index.distal.rotation = ConvertOrientationToLeap(Rotation);
		
				break;
			case EHandKeypoint::IndexTip:
				LeapHand.index.distal.next_joint = ConvertPositionToLeap(Position);
			
				break;
			// Middle ////////////////////////////////////////////////////
			case EHandKeypoint::MiddleMetacarpal:
				LeapHand.middle.metacarpal.prev_joint = ConvertPositionToLeap(Position);
				LeapHand.middle.metacarpal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::MiddleProximal:
				LeapHand.middle.proximal.prev_joint = LeapHand.middle.metacarpal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.middle.proximal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::MiddleIntermediate:
				LeapHand.middle.intermediate.prev_joint = LeapHand.middle.proximal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.middle.intermediate.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::MiddleDistal:
				LeapHand.middle.distal.prev_joint = LeapHand.middle.intermediate.next_joint = ConvertPositionToLeap(Position);
				LeapHand.middle.distal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::MiddleTip:
				LeapHand.middle.distal.next_joint = ConvertPositionToLeap(Position);

				break;
			// Ring ////////////////////////////////////////////////////
			case EHandKeypoint::RingMetacarpal:
				LeapHand.ring.metacarpal.prev_joint = ConvertPositionToLeap(Position);
				LeapHand.ring.metacarpal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::RingProximal:
				LeapHand.ring.proximal.prev_joint = LeapHand.ring.metacarpal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.ring.proximal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::RingIntermediate:
				LeapHand.ring.intermediate.prev_joint = LeapHand.ring.proximal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.ring.intermediate.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::RingDistal:
				LeapHand.ring.distal.prev_joint = LeapHand.ring.intermediate.next_joint = ConvertPositionToLeap(Position);
				LeapHand.ring.distal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::RingTip:
				LeapHand.ring.distal.next_joint = ConvertPositionToLeap(Position);

				break;

			// Little/pinky ////////////////////////////////////////////////////
			case EHandKeypoint::LittleMetacarpal:
				LeapHand.pinky.metacarpal.prev_joint = ConvertPositionToLeap(Position);
				LeapHand.pinky.metacarpal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::LittleProximal:
				LeapHand.pinky.proximal.prev_joint = LeapHand.pinky.metacarpal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.pinky.proximal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::LittleIntermediate:
				LeapHand.pinky.intermediate.prev_joint = LeapHand.pinky.proximal.next_joint = ConvertPositionToLeap(Position);
				LeapHand.pinky.intermediate.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::LittleDistal:
				LeapHand.pinky.distal.prev_joint = LeapHand.pinky.intermediate.next_joint = ConvertPositionToLeap(Position);
				LeapHand.pinky.distal.rotation = ConvertOrientationToLeap(Rotation);

				break;
			case EHandKeypoint::LittleTip:
				LeapHand.pinky.distal.next_joint = ConvertPositionToLeap(Position);

				break;
			default:
				break;
		}
		KeyPoint++;
	}

}
	
LEAP_TRACKING_EVENT* FOpenXRToLeapWrapper::GetInterpolatedFrameAtTime(int64 TimeStamp)
{
	return GetFrame();
}
void FOpenXRToLeapWrapper::UpdateHandState()
{

}
LEAP_DEVICE_INFO* FOpenXRToLeapWrapper::GetDeviceProperties()
{
	return CurrentDeviceInfo;
}
// TODO: update hand Ids on tracking lost and found
// TODO: work out hand poses and confidence based on how the MS/WMR code does it (pinch and grab)
LEAP_TRACKING_EVENT* FOpenXRToLeapWrapper::GetFrame()
{
	if (HandTracker == nullptr)
	{
		return &DummyLeapFrame;
	}
	FOccluderVertexArray OutPositions[2];
	TArray<FQuat> OutRotations[2];
	TArray<float> OutRadii[2];

	// status only true when the hand is being tracked/visible to the tracking device
	// IMPORTANT: OpenXR tracking only works in VR mode, this will always return false in desktop mode
	bool StatusLeft = HandTracker->GetAllKeypointStates(EControllerHand::Left, OutPositions[0], OutRotations[0], OutRadii[0]);
	bool StatusRight = HandTracker->GetAllKeypointStates(EControllerHand::Right, OutPositions[1], OutRotations[1], OutRadii[1]);

	DummyLeapFrame.nHands = StatusLeft + StatusRight;
	DummyLeapFrame.info.frame_id++;
	UWorld* World = nullptr;
	
	// time in microseconds
	DummyLeapFrame.info.timestamp = GetDummyLeapTime();
	DummyLeapFrame.tracking_frame_id++;

	if (!StatusLeft)
	{
		DummyLeapFrame.pHands = &DummyLeapHands[1];
	}
	else
	{
		DummyLeapFrame.pHands = &DummyLeapHands[0];
	}

	if (StatusLeft)
	{
		ConvertToLeapSpace(DummyLeapHands[0], OutPositions[0], OutRotations[0]);
	}
	if (StatusRight)
	{
		ConvertToLeapSpace(DummyLeapHands[1], OutPositions[1], OutRotations[1]);
	}
	return &DummyLeapFrame;
}
int64_t FOpenXRToLeapWrapper::GetDummyLeapTime()
{
	// time in microseconds
	if (!CurrentWorld)
	{
		return 0;
	}
	{
		return CurrentWorld->GetTimeSeconds() * 1000000.0f;
	}
}