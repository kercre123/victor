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
#include "anki/cozmo/MessageDefMacros_Robot.h"
#elif defined(COZMO_BASESTATION)
#include "anki/cozmo/MessageDefMacros_Basestation.h"
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

START_TIMESTAMPED_MESSAGE_DEFINITION(RobotState, 1)
ADD_MESSAGE_MEMBER(f32, pose_x)
ADD_MESSAGE_MEMBER(f32, pose_y)
ADD_MESSAGE_MEMBER(f32, pose_z)
ADD_MESSAGE_MEMBER(f32, pose_angle)
ADD_MESSAGE_MEMBER(f32, lwheel_speed_mmps)
ADD_MESSAGE_MEMBER(f32, rwheel_speed_mmps)
ADD_MESSAGE_MEMBER(f32, headAngle)
ADD_MESSAGE_MEMBER(f32, liftHeight)
// ...
END_MESSAGE_DEFINITION(RobotState)

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
ADD_MESSAGE_MEMBER(f32, height_mm)
ADD_MESSAGE_MEMBER(f32, max_speed_rad_per_sec)
ADD_MESSAGE_MEMBER(f32, accel_rad_per_sec2)
END_MESSAGE_DEFINITION(MoveLift)

// MoveHead
START_MESSAGE_DEFINITION(MoveHead, 1)
ADD_MESSAGE_MEMBER(f32, angle_rad)
ADD_MESSAGE_MEMBER(f32, max_speed_rad_per_sec)
ADD_MESSAGE_MEMBER(f32, accel_rad_per_sec2)
END_MESSAGE_DEFINITION(MoveHead)

// ClearPath
START_MESSAGE_DEFINITION(ClearPath, 1)
ADD_MESSAGE_MEMBER(u16, pathID)
END_MESSAGE_DEFINITION(ClearPath)

// Common members for PathSegment message definitions below:
#define ADD_COMMON_PATH_SEGMENT_MEMBERS \
ADD_MESSAGE_MEMBER(f32, targetSpeed) \
ADD_MESSAGE_MEMBER(f32, accel) \
ADD_MESSAGE_MEMBER(f32, decel) \
ADD_MESSAGE_MEMBER(u16, pathID) \
ADD_MESSAGE_MEMBER(u8,  segmentID)

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

// ExecutePath
START_MESSAGE_DEFINITION(ExecutePath, 1)
ADD_MESSAGE_MEMBER(u16, pathID)   // u16 or u8?
END_MESSAGE_DEFINITION(ExecutePath)

// VisionMarker
#define VISION_MARKER_CODE_LENGTH 11 // ceil( (9*9 + 4)/8 )

// DockToBlock
START_MESSAGE_DEFINITION(DockWithBlock, 1)
ADD_MESSAGE_MEMBER(f32, horizontalOffset_mm)  // Offset wrt to docking block. Don't know if this will work yet.
ADD_MESSAGE_MEMBER(f32, markerWidth_mm)
ADD_MESSAGE_MEMBER(u8, dockAction)  // See DockAction_t
ADD_MESSAGE_MEMBER_ARRAY(u8, blockCode, VISION_MARKER_CODE_LENGTH)
END_MESSAGE_DEFINITION(DockWithBlock)


START_TIMESTAMPED_MESSAGE_DEFINITION(VisionMarker, 1)
// TODO: make the corner coordinates fixed point
ADD_MESSAGE_MEMBER(f32, x_imgUpperLeft)
ADD_MESSAGE_MEMBER(f32, y_imgUpperLeft)
ADD_MESSAGE_MEMBER(f32, x_imgLowerLeft)
ADD_MESSAGE_MEMBER(f32, y_imgLowerLeft)
ADD_MESSAGE_MEMBER(f32, x_imgUpperRight)
ADD_MESSAGE_MEMBER(f32, y_imgUpperRight)
ADD_MESSAGE_MEMBER(f32, x_imgLowerRight)
ADD_MESSAGE_MEMBER(f32, y_imgLowerRight)
ADD_MESSAGE_MEMBER_ARRAY(u8, code, VISION_MARKER_CODE_LENGTH)
END_MESSAGE_DEFINITION(VisionMarker)

// BlockMarkerObserved
// TODO: this has to be split into two packets for BTLE (size > 20 bytes)
START_TIMESTAMPED_MESSAGE_DEFINITION(BlockMarkerObserved, 1)

  // TODO: u16 frameNum; // for putting together two halves of a BlockMarker packet
ADD_MESSAGE_MEMBER(f32, headAngle)  // TODO: should this be it's own message, only when changed?
// TODO: these need to be fixed-point, probably 16bits
ADD_MESSAGE_MEMBER(f32, x_imgUpperLeft)
ADD_MESSAGE_MEMBER(f32, y_imgUpperLeft)
ADD_MESSAGE_MEMBER(f32, x_imgLowerLeft)
ADD_MESSAGE_MEMBER(f32, y_imgLowerLeft)
ADD_MESSAGE_MEMBER(f32, x_imgUpperRight)
ADD_MESSAGE_MEMBER(f32, y_imgUpperRight)
ADD_MESSAGE_MEMBER(f32, x_imgLowerRight)
ADD_MESSAGE_MEMBER(f32, y_imgLowerRight)
ADD_MESSAGE_MEMBER(u16, blockType)
ADD_MESSAGE_MEMBER(u8,  faceType)
ADD_MESSAGE_MEMBER(u8,  upDirection)
END_MESSAGE_DEFINITION(BlockMarkerObserved)

// MatMarkerObserved
START_TIMESTAMPED_MESSAGE_DEFINITION(MatMarkerObserved, 1)
ADD_MESSAGE_MEMBER(f32, x_imgCenter)   // Where in the image we saw it
ADD_MESSAGE_MEMBER(f32, y_imgCenter)   //    "
ADD_MESSAGE_MEMBER(f32, angle)         // What angle in the image we saw it
ADD_MESSAGE_MEMBER(u16, x_MatSquare)   // Which Mat square we saw
ADD_MESSAGE_MEMBER(u16, y_MatSquare)   //    "
ADD_MESSAGE_MEMBER(u8,  upDirection)
END_MESSAGE_DEFINITION(MatMarkerObserved)

// DockingErrorSignal
START_TIMESTAMPED_MESSAGE_DEFINITION(DockingErrorSignal, 1)
ADD_MESSAGE_MEMBER(f32, x_distErr)
ADD_MESSAGE_MEMBER(f32, y_horErr)
ADD_MESSAGE_MEMBER(f32, angleErr) // in radians
ADD_MESSAGE_MEMBER(u8,  didTrackingSucceed)
END_MESSAGE_DEFINITION(DockingErrorSignal)

// AbsLocalizationUpdate
START_TIMESTAMPED_MESSAGE_DEFINITION(AbsLocalizationUpdate, 1)
ADD_MESSAGE_MEMBER(f32, xPosition)
ADD_MESSAGE_MEMBER(f32, yPosition)
ADD_MESSAGE_MEMBER(f32, headingAngle)
END_MESSAGE_DEFINITION(AbsLocalizationUpdate)


// Common Camera Calibration Message Members:
// TODO: Remove fov?  (it can be computed from focal length)
// TODO: Assume zero skew and remove that member?
#define ADD_COMMON_CAMERA_CALIBRATION_MEMBERS \
ADD_MESSAGE_MEMBER(f32, focalLength_x) \
ADD_MESSAGE_MEMBER(f32, focalLength_y) \
ADD_MESSAGE_MEMBER(f32, fov) \
ADD_MESSAGE_MEMBER(f32, center_x) \
ADD_MESSAGE_MEMBER(f32, center_y) \
ADD_MESSAGE_MEMBER(f32, skew) \
ADD_MESSAGE_MEMBER(u16, nrows) \
ADD_MESSAGE_MEMBER(u16, ncols)

// HeadCameraCalibration
START_MESSAGE_DEFINITION(HeadCameraCalibration, 1)
ADD_COMMON_CAMERA_CALIBRATION_MEMBERS
END_MESSAGE_DEFINITION(HeadCameraCalibration)

// MatCameraCalibration
START_MESSAGE_DEFINITION(MatCameraCalibration, 1)
ADD_COMMON_CAMERA_CALIBRATION_MEMBERS
END_MESSAGE_DEFINITION(MatCameraCalibration)

// Robot Available
START_MESSAGE_DEFINITION(RobotAvailable, 1)
ADD_MESSAGE_MEMBER(u32, robotID)
// TODO: Add other members here?
END_MESSAGE_DEFINITION(RobotAvailable)

// RobotAddedToWorld
START_MESSAGE_DEFINITION(RobotAddedToWorld, 1)
ADD_MESSAGE_MEMBER(u32, robotID)
// TODO: Add other members here?
END_MESSAGE_DEFINITION(RobotAddedToWorld)

// TemplateInitialized
START_MESSAGE_DEFINITION(TemplateInitialized, 1)
ADD_MESSAGE_MEMBER(u8, success)
END_MESSAGE_DEFINITION(TemplateInitialized)

// TotalBlocksDetected
START_MESSAGE_DEFINITION(TotalVisionMarkersSeen, 1)
ADD_MESSAGE_MEMBER(u8, numMarkers)
END_MESSAGE_DEFINITION(TotalVisionMarkersSeen)


// PrintText
#define PRINT_TEXT_MSG_LENGTH 50
START_MESSAGE_DEFINITION(PrintText, 1)
ADD_MESSAGE_MEMBER_ARRAY(u8, text, PRINT_TEXT_MSG_LENGTH)
END_MESSAGE_DEFINITION(PrintText)







