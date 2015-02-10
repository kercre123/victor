// All message definitions should look like this:
//
//   START_MESSAGE_DEFINITION(MessageTypeName, Priority)
//   ADD_MESSAGE_MEMBER(MemberType, MemberName)
//   ADD_MESSAGE_MEMBER(MemberType, MemberName)
//   ...
//   END_MESSAGE_DEFINITION(MessageTypeName)
//
// For example:
//
//   START_MESSAGE_DEFINITION(CozmoMsg_Foo, 1)
//   ADD_MESSAGE_MEMBER(f32, fooMember1)
//   ADD_MESSAGE_MEMBER(u16, fooMember2)
//   ADD_MESSAGE_MEMBER(u8,  fooMember3)
//   END_MESSAGE_DEFINITION(CozmoMsg_Foo)
//
// To add a timestamp member, use the START_TIMESTAMPED_MESSAGE_DEFINITION
// command, which will add a member "timestamp" to the struct, with type
// "TimeStamp".
//
// IMPORTANT NOTE: You should always add members from largest to smallest type!
//                 (This prevents memory alignment badness when doing memcopies
//                  or casts later.)
//

// This file is not specific to robot or basestation, as it defines the common
// message protocol between them.  However, the macros used to generate actual
// code based on the definitions below *is* specific to the two platforms.
// Include the correct one based on the definition of COZMO_ROBOT / COZMO_BASESTATION
#if defined(COZMO_ROBOT)
#error UIMessages should not be used on robot!
#elif defined(COZMO_BASESTATION)
#include "anki/cozmo/shared/MessageDefMacros_Basestation.h"
#else
#error COZMO_BASESTATION should be defined to use UIMessageDefinitions
#endif

#if 0 // EXAMPLE
// Foo message
START_MESSAGE_DEFINITION(Foo, 1)
ADD_MESSAGE_MEMBER(f32, fooMember1)
ADD_MESSAGE_MEMBER(u16, fooMember2)
ADD_MESSAGE_MEMBER(u8,  fooMember3)
END_MESSAGE_DEFINITION(Foo)
#endif

// ConnectToRobot
START_MESSAGE_DEFINITION(U2G_ConnectToRobot, 1)
ADD_MESSAGE_MEMBER(u8, robotID)
END_MESSAGE_DEFINITION(U2G_ConnectToRobot)

// ConnectToUiDevice
START_MESSAGE_DEFINITION(U2G_ConnectToUiDevice, 1)
ADD_MESSAGE_MEMBER(u8, deviceID)
END_MESSAGE_DEFINITION(U2G_ConnectToUiDevice)

// ForceAddRobot
// NOTE: This can be removed once physical robots can advertise (e.g. with BLE)
START_MESSAGE_DEFINITION(U2G_ForceAddRobot, 1)
ADD_MESSAGE_MEMBER_ARRAY(u8, ipAddress, 16)
ADD_MESSAGE_MEMBER(u8, robotID)
ADD_MESSAGE_MEMBER(u8, isSimulated)
END_MESSAGE_DEFINITION(U2G_ForceAddRobot)

// Heartbeat
START_MESSAGE_DEFINITION(U2G_Heartbeat, 1)
ADD_MESSAGE_MEMBER(f32, currentTime_sec)
END_MESSAGE_DEFINITION(U2G_Heartbeat)

// DriveWheels
START_MESSAGE_DEFINITION(U2G_DriveWheels, 1)
ADD_MESSAGE_MEMBER(f32, lwheel_speed_mmps)
ADD_MESSAGE_MEMBER(f32, rwheel_speed_mmps)
END_MESSAGE_DEFINITION(U2G_DriveWheels)

// MoveHead
START_MESSAGE_DEFINITION(U2G_MoveHead, 1)
ADD_MESSAGE_MEMBER(f32, speed_rad_per_sec)
END_MESSAGE_DEFINITION(U2G_MoveHead)

// MoveLift
START_MESSAGE_DEFINITION(U2G_MoveLift, 1)
ADD_MESSAGE_MEMBER(f32, speed_rad_per_sec)
END_MESSAGE_DEFINITION(U2G_MoveLift)

// SetLiftHeight
START_MESSAGE_DEFINITION(U2G_SetLiftHeight, 1)
ADD_MESSAGE_MEMBER(f32, height_mm)
ADD_MESSAGE_MEMBER(f32, max_speed_rad_per_sec)
ADD_MESSAGE_MEMBER(f32, accel_rad_per_sec2)
END_MESSAGE_DEFINITION(U2G_SetLiftHeight)

// SetHeadAngle
START_MESSAGE_DEFINITION(U2G_SetHeadAngle, 1)
ADD_MESSAGE_MEMBER(f32, angle_rad)
ADD_MESSAGE_MEMBER(f32, max_speed_rad_per_sec)
ADD_MESSAGE_MEMBER(f32, accel_rad_per_sec2)
END_MESSAGE_DEFINITION(U2G_SetHeadAngle)

// StopAllMotors
START_MESSAGE_DEFINITION(U2G_StopAllMotors, 1)
END_MESSAGE_DEFINITION(U2G_StopAllMotors)

// ImageRequest
START_MESSAGE_DEFINITION(U2G_ImageRequest, 1)
ADD_MESSAGE_MEMBER(u8, mode)
ADD_MESSAGE_MEMBER(u8, resolution)
END_MESSAGE_DEFINITION(U2G_ImageRequest)

// SaveImages
START_MESSAGE_DEFINITION(U2G_SaveImages, 1)
ADD_MESSAGE_MEMBER(u8, enableSave)
END_MESSAGE_DEFINITION(U2G_SaveImages)

// EnableDisplay
START_MESSAGE_DEFINITION(U2G_EnableDisplay, 1)
ADD_MESSAGE_MEMBER(u8, enable)
END_MESSAGE_DEFINITION(U2G_EnableDisplay)

// SetHeadlights
START_MESSAGE_DEFINITION(U2G_SetHeadlights, 1)
ADD_MESSAGE_MEMBER(u8, intensity)
END_MESSAGE_DEFINITION(U2G_SetHeadlights)

// GotoPose
START_MESSAGE_DEFINITION(U2G_GotoPose, 1)
ADD_MESSAGE_MEMBER(f32, x_mm)
ADD_MESSAGE_MEMBER(f32, y_mm)
ADD_MESSAGE_MEMBER(f32, rad)
ADD_MESSAGE_MEMBER(u8, level)
END_MESSAGE_DEFINITION(U2G_GotoPose)

// PlaceObjectOnGround
START_MESSAGE_DEFINITION(U2G_PlaceObjectOnGround, 1)
ADD_MESSAGE_MEMBER(f32, x_mm)
ADD_MESSAGE_MEMBER(f32, y_mm)
ADD_MESSAGE_MEMBER(f32, rad)
ADD_MESSAGE_MEMBER(u8, level)
END_MESSAGE_DEFINITION(U2G_PlaceObjectOnGround)

// PlaceObjectOnGroundHere
START_MESSAGE_DEFINITION(U2G_PlaceObjectOnGroundHere, 1)
END_MESSAGE_DEFINITION(U2G_PlaceObjectOnGroundHere)

// ExecuteTestPlan
START_MESSAGE_DEFINITION(U2G_ExecuteTestPlan, 1)
END_MESSAGE_DEFINITION(U2G_ExecuteTestPlan)

// SelectNextObject
START_MESSAGE_DEFINITION(U2G_SelectNextObject, 1)
END_MESSAGE_DEFINITION(U2G_SelectNextObject)

// PickAndPlaceObject
START_MESSAGE_DEFINITION(U2G_PickAndPlaceObject, 1)
ADD_MESSAGE_MEMBER(s32, objectID) // negative value means "currently selected object"
ADD_MESSAGE_MEMBER(u8, usePreDockPose)
END_MESSAGE_DEFINITION(U2G_PickAndPlaceObject)

// TraverseObject
START_MESSAGE_DEFINITION(U2G_TraverseObject, 1)
END_MESSAGE_DEFINITION(U2G_TraverseObject)

// ClearAllBlocks
START_MESSAGE_DEFINITION(U2G_ClearAllBlocks, 1)
END_MESSAGE_DEFINITION(U2G_ClearAllBlocks)

// ExecuteBehavior
START_MESSAGE_DEFINITION(U2G_ExecuteBehavior, 1)
ADD_MESSAGE_MEMBER(u8, behaviorMode)
END_MESSAGE_DEFINITION(U2G_ExecuteBehavior)

// SetBehaviorState
START_MESSAGE_DEFINITION(U2G_SetBehaviorState, 1)
ADD_MESSAGE_MEMBER(u8, behaviorState)
END_MESSAGE_DEFINITION(U2G_SetBehaviorState)

// AbortPath
START_MESSAGE_DEFINITION(U2G_AbortPath, 1)
END_MESSAGE_DEFINITION(U2G_AbortPath)

// AbortAll
START_MESSAGE_DEFINITION(U2G_AbortAll, 1)
END_MESSAGE_DEFINITION(U2G_AbortAll)

// DrawPoseMarker
START_MESSAGE_DEFINITION(U2G_DrawPoseMarker, 1)
ADD_MESSAGE_MEMBER(f32, x_mm)
ADD_MESSAGE_MEMBER(f32, y_mm)
ADD_MESSAGE_MEMBER(f32, rad)
ADD_MESSAGE_MEMBER(u8, level)
END_MESSAGE_DEFINITION(U2G_DrawPoseMarker)

// ErasePoseMarker
START_MESSAGE_DEFINITION(U2G_ErasePoseMarker, 1)
END_MESSAGE_DEFINITION(U2G_ErasePoseMarker)

// SetHeadControllerGains
START_MESSAGE_DEFINITION(U2G_SetHeadControllerGains, 1)
ADD_MESSAGE_MEMBER(f32, kp)
ADD_MESSAGE_MEMBER(f32, ki)
ADD_MESSAGE_MEMBER(f32, maxIntegralError)
END_MESSAGE_DEFINITION(U2G_SetHeadControllerGains)

// SetLiftControllerGains
START_MESSAGE_DEFINITION(U2G_SetLiftControllerGains, 1)
ADD_MESSAGE_MEMBER(f32, kp)
ADD_MESSAGE_MEMBER(f32, ki)
ADD_MESSAGE_MEMBER(f32, maxIntegralError)
END_MESSAGE_DEFINITION(U2G_SetLiftControllerGains)

// SelectNextSoundScheme
START_MESSAGE_DEFINITION(U2G_SelectNextSoundScheme, 1)
END_MESSAGE_DEFINITION(U2G_SelectNextSoundScheme)

// StartTestMode
START_MESSAGE_DEFINITION(U2G_StartTestMode, 1)
ADD_MESSAGE_MEMBER(s32, p1)
ADD_MESSAGE_MEMBER(s32, p2)
ADD_MESSAGE_MEMBER(s32, p3)
ADD_MESSAGE_MEMBER(u8, mode)
END_MESSAGE_DEFINITION(U2G_StartTestMode)

// IMURequest
START_MESSAGE_DEFINITION(U2G_IMURequest, 1)
ADD_MESSAGE_MEMBER(u32, length_ms)
END_MESSAGE_DEFINITION(U2G_IMURequest)

// PlayAnimation
START_MESSAGE_DEFINITION(U2G_PlayAnimation, 1)
ADD_MESSAGE_MEMBER(u32, numLoops)
//ADD_MESSAGE_MEMBER(u8, animationID)
ADD_MESSAGE_MEMBER_ARRAY(char, animationName, 32) // TODO: don't use char arrays
END_MESSAGE_DEFINITION(U2G_PlayAnimation)

// ReadAnimationFile
START_MESSAGE_DEFINITION(U2G_ReadAnimationFile, 1)
END_MESSAGE_DEFINITION(U2G_ReadAnimationFile)

// StartFaceTracking
START_MESSAGE_DEFINITION(U2G_StartFaceTracking, 1)
//TODO: add ADD_MESSAGE_MEMBER(u8, selectionMode)
ADD_MESSAGE_MEMBER(u8, timeout_sec)
END_MESSAGE_DEFINITION(U2G_StartFaceTracking)

// StopFaceTracking
START_MESSAGE_DEFINITION(U2G_StopFaceTracking, 1)
END_MESSAGE_DEFINITION(U2G_StopFaceTracking)

// StartLookingForMarkers
START_MESSAGE_DEFINITION(U2G_StartLookingForMarkers, 1)
END_MESSAGE_DEFINITION(U2G_StartLookingForMarkers)

// StopLookingForMarkers
START_MESSAGE_DEFINITION(U2G_StopLookingForMarkers, 1)
END_MESSAGE_DEFINITION(U2G_StopLookingForMarkers)

// SetVisionSystemParams
START_MESSAGE_DEFINITION(U2G_SetVisionSystemParams, 1)
ADD_MESSAGE_MEMBER(s32, autoexposureOn)
ADD_MESSAGE_MEMBER(f32, exposureTime)
ADD_MESSAGE_MEMBER(s32, integerCountsIncrement)
ADD_MESSAGE_MEMBER(f32, minExposureTime)
ADD_MESSAGE_MEMBER(f32, maxExposureTime)
ADD_MESSAGE_MEMBER(f32, percentileToMakeHigh)
ADD_MESSAGE_MEMBER(s32, limitFramerate)
ADD_MESSAGE_MEMBER(u8, highValue)
END_MESSAGE_DEFINITION(U2G_SetVisionSystemParams)

// SetFaceDetectParams
START_MESSAGE_DEFINITION(U2G_SetFaceDetectParams, 1)
ADD_MESSAGE_MEMBER(f32, scaleFactor)
ADD_MESSAGE_MEMBER(s32, minNeighbors)
ADD_MESSAGE_MEMBER(s32, minObjectHeight)
ADD_MESSAGE_MEMBER(s32, minObjectWidth)
ADD_MESSAGE_MEMBER(s32, maxObjectHeight)
ADD_MESSAGE_MEMBER(s32, maxObjectWidth)
END_MESSAGE_DEFINITION(U2G_SetFaceDetectParams)


