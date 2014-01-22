// All message definitions should look like this:
//
//   START_MESSAGE_DEFINITION(MessageTypeName, Priority, DispatchFunctionPtr)
//   ADD_MESSAGE_MEMBER(MemberType, MemberName)
//   ADD_MESSAGE_MEMBER(MemberType, MemberName)
//   ...
//   END_MESSAGE_DEFINITION(MessageTypeName)
//
// For example:
//
//   START_MESSAGE_DEFINITION(CozmoMsg_Foo, 1, ProcessFoo)
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
// Define those macros elsewhere:
#include "anki/cozmo/MessageDefinitionMacros.h"

#if 0 // EXAMPLE
// Foo message
START_MESSAGE_DEFINITION(Foo, 1)
ADD_MESSAGE_MEMBER(f32, fooMember1)
ADD_MESSAGE_MEMBER(u16, fooMember2)
ADD_MESSAGE_MEMBER(u8,  fooMember3)
END_MESSAGE_DEFINITION(Foo)
#endif 

// SetMotion
START_MESSAGE_DEFINITION(SetMotion, 1)
ADD_MESSAGE_MEMBER(s16, speed_mmPerSec)      // Commanded speed in mm/sec
ADD_MESSAGE_MEMBER(s16, accel_mmPerSec2)     // Commanded max absolute value of...
                                             // ...acceleration/decceleration in mm/sec^2
ADD_MESSAGE_MEMBER(s16, curvatureRadius_mm)  // +ve: curves left, -ve: curves right, ...
                                             // ...u16_MAX: point turn left, 16_MIN: point ...
                                             // ...turn right, 0: straight
END_MESSAGE_DEFINITION(SetMotion)

// ClearPath
START_MESSAGE_DEFINITION(ClearPath, 1)
ADD_MESSAGE_MEMBER(u16, pathID)
END_MESSAGE_DEFINITION(ClearPath)

// Common members for PathSegment message definitions below:
#define ADD_COMMON_PATH_SEGMENT_MEMBERS \
ADD_MESSAGE_MEMBER(u16, pathID) \
ADD_MESSAGE_MEMBER(s16, desiredSpeed_mmPerSec) \
ADD_MESSAGE_MEMBER(s16, terminalSpeed_mmPerSec) \
ADD_MESSAGE_MEMBER(u8,  segmentID)

// SetPathSegmentLine
START_MESSAGE_DEFINITION(SetPathSegmentLine, 1)
ADD_MESSAGE_MEMBER(f32, x_start_m)
ADD_MESSAGE_MEMBER(f32, y_start_m)
ADD_MESSAGE_MEMBER(f32, x_end_m)
ADD_MESSAGE_MEMBER(f32, y_end_m)
ADD_COMMON_PATH_SEGMENT_MEMBERS
END_MESSAGE_DEFINITION(SetPathSegmentLine)

// SetPathSegmentArc
START_MESSAGE_DEFINITION(SetPathSegmentArc, 1)
ADD_MESSAGE_MEMBER(f32, x_center_m)
ADD_MESSAGE_MEMBER(f32, y_center_m)
ADD_MESSAGE_MEMBER(f32, radius_m)
ADD_MESSAGE_MEMBER(f32, startRad)
ADD_MESSAGE_MEMBER(f32, endRad)
ADD_COMMON_PATH_SEGMENT_MEMBERS
END_MESSAGE_DEFINITION(SetPathSegmentArc)


// VisionMarker
#define VISION_MARKER_CODE_LENGTH 11 // ceil( (9*9 + 4)/8 )

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






