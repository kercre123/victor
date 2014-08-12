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
#include "anki/cozmo/shared/MessageDefMacros_Robot.h"
#elif defined(COZMO_BASESTATION)
#include "anki/cozmo/shared/MessageDefMacros_Basestation.h"
#else
#error Either COZMO_ROBOT or COZMO_BASESTATION should be defined.
#endif

#if 0 // EXAMPLE
// Foo message
START_MESSAGE_DEFINITION(Foo, 1)
ADD_MESSAGE_MEMBER(f32, fooMember1)
ADD_MESSAGE_MEMBER(u16, fooMember2)
ADD_MESSAGE_MEMBER(u8,  fooMember3)
END_MESSAGE_DEFINITION(Foo)
#endif 

// DriveWheels
START_MESSAGE_DEFINITION(DriveWheels, 1)
ADD_MESSAGE_MEMBER(f32, lwheel_speed_mmps)
ADD_MESSAGE_MEMBER(f32, rwheel_speed_mmps)
END_MESSAGE_DEFINITION(DriveWheels)

// DriveWheelsCurvature
START_MESSAGE_DEFINITION(DriveWheelsCurvature, 1)
ADD_MESSAGE_MEMBER(s16, speed_mmPerSec)      // Commanded speed in mm/sec
ADD_MESSAGE_MEMBER(u16, accel_mmPerSec2)     // Commanded max absolute value of...
ADD_MESSAGE_MEMBER(u16, decel_mmPerSec2)     // ...acceleration/deceleration in mm/sec^2
ADD_MESSAGE_MEMBER(s16, curvatureRadius_mm)  // +ve: curves left, -ve: curves right, ...
                                             // ...u16_MAX: point turn left, 16_MIN: point ...
                                             // ...turn right, 0: straight
END_MESSAGE_DEFINITION(DriveWheelsCurvature)


// MoveLift
START_MESSAGE_DEFINITION(MoveLift, 1)
ADD_MESSAGE_MEMBER(f32, speed_rad_per_sec)
END_MESSAGE_DEFINITION(MoveLift)

// MoveHead
START_MESSAGE_DEFINITION(MoveHead, 1)
ADD_MESSAGE_MEMBER(f32, speed_rad_per_sec)
END_MESSAGE_DEFINITION(MoveHead)


// SetLiftHeight
START_MESSAGE_DEFINITION(SetLiftHeight, 1)
ADD_MESSAGE_MEMBER(f32, height_mm)
ADD_MESSAGE_MEMBER(f32, max_speed_rad_per_sec)
ADD_MESSAGE_MEMBER(f32, accel_rad_per_sec2)
END_MESSAGE_DEFINITION(SetLiftHeight)

// SetHeadAngle
START_MESSAGE_DEFINITION(SetHeadAngle, 1)
ADD_MESSAGE_MEMBER(f32, angle_rad)
ADD_MESSAGE_MEMBER(f32, max_speed_rad_per_sec)
ADD_MESSAGE_MEMBER(f32, accel_rad_per_sec2)
END_MESSAGE_DEFINITION(SetHeadAngle)

// StopAllMotors
START_MESSAGE_DEFINITION(StopAllMotors, 1)
END_MESSAGE_DEFINITION(StopAllMotors)

// ClearPath
START_MESSAGE_DEFINITION(ClearPath, 1)
ADD_MESSAGE_MEMBER(u16, pathID)
END_MESSAGE_DEFINITION(ClearPath)

// Common members for PathSegment message definitions below:
#define ADD_COMMON_PATH_SEGMENT_MEMBERS \
ADD_MESSAGE_MEMBER(f32, targetSpeed) \
ADD_MESSAGE_MEMBER(f32, accel) \
ADD_MESSAGE_MEMBER(f32, decel)

// AppendPathSegmentLine
START_MESSAGE_DEFINITION(AppendPathSegmentLine, 1)
ADD_MESSAGE_MEMBER(f32, x_start_mm)
ADD_MESSAGE_MEMBER(f32, y_start_mm)
ADD_MESSAGE_MEMBER(f32, x_end_mm)
ADD_MESSAGE_MEMBER(f32, y_end_mm)
ADD_COMMON_PATH_SEGMENT_MEMBERS
END_MESSAGE_DEFINITION(AppendPathSegmentLine)

// AppendPathSegmentArc
START_MESSAGE_DEFINITION(AppendPathSegmentArc, 1)
ADD_MESSAGE_MEMBER(f32, x_center_mm)
ADD_MESSAGE_MEMBER(f32, y_center_mm)
ADD_MESSAGE_MEMBER(f32, radius_mm)
ADD_MESSAGE_MEMBER(f32, startRad)
ADD_MESSAGE_MEMBER(f32, sweepRad)
ADD_COMMON_PATH_SEGMENT_MEMBERS
END_MESSAGE_DEFINITION(AppendPathSegmentArc)

// AppendPathSegmentPointTurn
START_MESSAGE_DEFINITION(AppendPathSegmentPointTurn, 1)
ADD_MESSAGE_MEMBER(f32, x_center_mm)
ADD_MESSAGE_MEMBER(f32, y_center_mm)
ADD_MESSAGE_MEMBER(f32, targetRad)
ADD_COMMON_PATH_SEGMENT_MEMBERS  // targetSpeed indicates rotational speed in rad/s
END_MESSAGE_DEFINITION(AppendPathSegmentPointTurn)

// TrimPath
START_MESSAGE_DEFINITION(TrimPath, 1)
ADD_MESSAGE_MEMBER(u8, numPopFrontSegments)
ADD_MESSAGE_MEMBER(u8, numPopBackSegments)
END_MESSAGE_DEFINITION(TrimPath)

// ExecutePath
START_MESSAGE_DEFINITION(ExecutePath, 1)
ADD_MESSAGE_MEMBER(u16, pathID)   // u16 or u8?
END_MESSAGE_DEFINITION(ExecutePath)

// DockWithObject
START_MESSAGE_DEFINITION(DockWithObject, 1)
ADD_MESSAGE_MEMBER(f32, horizontalOffset_mm)  // Offset wrt to docking object. Don't know if this will work yet.
ADD_MESSAGE_MEMBER(f32, markerWidth_mm)
ADD_MESSAGE_MEMBER(u16, image_pixel_x)
ADD_MESSAGE_MEMBER(u16, image_pixel_y)
ADD_MESSAGE_MEMBER(u8, pixel_radius)  // Marker must be found within this many pixels of specified coordinates,
                                      // unless pixel_radius == u8_MAX in which case marker may be located anywhere.
ADD_MESSAGE_MEMBER(u8, dockAction)  // See DockAction_t
ADD_MESSAGE_MEMBER(u8, markerType)
ADD_MESSAGE_MEMBER(u8, markerType2) // End marker (for bridge crossing only)
END_MESSAGE_DEFINITION(DockWithObject)

// PlaceObjectOnGround
START_MESSAGE_DEFINITION(PlaceObjectOnGround, 1)
ADD_MESSAGE_MEMBER(f32, rel_x_mm)    // Distance of object face center in forward axis
ADD_MESSAGE_MEMBER(f32, rel_y_mm)    // Distance of object face center in horizontal axis. (Left of robot is +ve)
ADD_MESSAGE_MEMBER(f32, rel_angle)   // Angle between of object face normal and robot. (Block normal pointing right of robot is +ve)
END_MESSAGE_DEFINITION(PlaceObjectOnGround)


// AbsLocalizationUpdate
START_TIMESTAMPED_MESSAGE_DEFINITION(AbsLocalizationUpdate, 1)
ADD_MESSAGE_MEMBER(u32, pose_frame_id)
ADD_MESSAGE_MEMBER(f32, xPosition)
ADD_MESSAGE_MEMBER(f32, yPosition)
ADD_MESSAGE_MEMBER(f32, headingAngle)
END_MESSAGE_DEFINITION(AbsLocalizationUpdate)

// RobotInit
START_MESSAGE_DEFINITION(RobotInit, 1)
ADD_MESSAGE_MEMBER(u32, robotID)
ADD_MESSAGE_MEMBER(u32, syncTime)
// TODO: Add other members here?
END_MESSAGE_DEFINITION(RobotInit)

/*
// TemplateInitialized
START_MESSAGE_DEFINITION(TemplateInitialized, 1)
ADD_MESSAGE_MEMBER(u8, success)
END_MESSAGE_DEFINITION(TemplateInitialized)

// TotalBlocksDetected
START_MESSAGE_DEFINITION(TotalVisionMarkersSeen, 1)
ADD_MESSAGE_MEMBER(u8, numMarkers)
END_MESSAGE_DEFINITION(TotalVisionMarkersSeen)
*/

// HeadAngleUpdate
START_MESSAGE_DEFINITION(HeadAngleUpdate, 1)
ADD_MESSAGE_MEMBER(f32, newAngle)
END_MESSAGE_DEFINITION(HeadAngleUpdate)

// ImageRequest
START_MESSAGE_DEFINITION(ImageRequest, 1)
ADD_MESSAGE_MEMBER(u8, imageSendMode)
ADD_MESSAGE_MEMBER(u8, resolution)
END_MESSAGE_DEFINITION(ImageRequest)

// StartTestMode
START_MESSAGE_DEFINITION(StartTestMode, 1)
ADD_MESSAGE_MEMBER(u8, mode)
END_MESSAGE_DEFINITION(StartTestMode)

// SetHeadlight
START_MESSAGE_DEFINITION(SetHeadlight, 1)
ADD_MESSAGE_MEMBER(u8, intensity)
END_MESSAGE_DEFINITION(SetHeadlight)

// SetDefaultLights
START_MESSAGE_DEFINITION(SetDefaultLights, 1)
ADD_MESSAGE_MEMBER(u32, eye_left_color)
ADD_MESSAGE_MEMBER(u32, eye_right_color)
END_MESSAGE_DEFINITION(SetDefaultLights)

// SetHeadControllerGains
START_MESSAGE_DEFINITION(SetHeadControllerGains, 1)
ADD_MESSAGE_MEMBER(f32, kp)
ADD_MESSAGE_MEMBER(f32, ki)
ADD_MESSAGE_MEMBER(f32, maxIntegralError)
END_MESSAGE_DEFINITION(SetHeadControllerGains)

// SetLiftControllerGains
START_MESSAGE_DEFINITION(SetLiftControllerGains, 1)
ADD_MESSAGE_MEMBER(f32, kp)
ADD_MESSAGE_MEMBER(f32, ki)
ADD_MESSAGE_MEMBER(f32, maxIntegralError)
END_MESSAGE_DEFINITION(SetLiftControllerGains)

// SetVisionSystemParams
START_MESSAGE_DEFINITION(SetVisionSystemParams, 1)
ADD_MESSAGE_MEMBER(s32, integerCountsIncrement)
ADD_MESSAGE_MEMBER(f32, minExposureTime)
ADD_MESSAGE_MEMBER(f32, maxExposureTime)
ADD_MESSAGE_MEMBER(f32, percentileToMakeHigh)
ADD_MESSAGE_MEMBER(u8, highValue)
END_MESSAGE_DEFINITION(SetVisionSystemParams)

// PlayAnimation
START_MESSAGE_DEFINITION(PlayAnimation, 1)
ADD_MESSAGE_MEMBER(u32, numLoops)
ADD_MESSAGE_MEMBER(u8, animationID)
END_MESSAGE_DEFINITION(PlayAnimation)

// IMURequest
START_MESSAGE_DEFINITION(IMURequest, 1)
ADD_MESSAGE_MEMBER(u32, length_ms)
END_MESSAGE_DEFINITION(IMURequest)
