/**
 * File: playpenConfig.h
 *
 * Author: Al Chaussee
 * Created: 08/10/17
 *
 * Description: Configuration file for playpen
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_PlaypenConfig__
#define __Cozmo_Basestation_PlaypenConfig__

#include "anki/common/types.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/types/customObjectMarkers.h"
#include "clad/types/robotStatusAndActions.h"

#include "util/console/consoleVariable.h"

namespace Anki {

namespace Cozmo {
  
namespace PlaypenConfig {

// ----------General----------
// Whether or not to disconnect from robot at end of tests
static bool  kDisconnectAtEnd      = false;

// Whether or not to write to robot storage
static bool  kWriteToStorage       = false;

// Whether or not to ignore any test failures
// The playpen behaviors should be written in such a way so that they can continue running
// even after something has gone wrong (basically no branching)
static bool  kIgnoreFailures       = true;

// Default timeout to force a playpen behavior to end
static f32   kDefaultTimeout_ms    = 20000;

// How long to display the playpen result on Cozmo's face
static f32 kTimeToDisplayResultOnFace_ms = 10000;

// ----------InitChecks----------
// Whether or not to check firmware version
static bool      kCheckFirmwareVersion = true;

// Minimum battery voltage the robot should have at the start and end of playpen
static f32       kMinBatteryVoltage    = 3.6;

// Minimum firmare version we are looking for
static u32       kMinFirmwareVersion   = 10501;

// Minimum hardware version we are looking for
static int       kMinHardwareVersion   = 3;

// Timeout to wait for MfgID message back from robot after requesting it
static u32       kMfgIDTimeout_ms      = 500;

// Minimum body color we are looking for
static BodyColor kMinBodyColor         = BodyColor::WHITE_v15;

// ----------Motor Calibration----------
// Timeout to wait for motor calibration to complete
static u32 kMotorCalibrationTimeout_ms = 4000;

// ----------Drift Check----------
// Head angle at which we should play the sound for both speaker and mic check
static f32 kHeadAngleToPlaySound    = DEG_TO_RAD(2.f);

// How long to check drift for
static u32 kIMUDriftDetectPeriod_ms = 2000;

// How much our heading can change over kIMUDriftDetectPeriod_ms
static f32 kIMUDriftAngleThreshDeg  = 0.2f;

// ----------Distance Sensor----------
// Number of distance sensor readings to record
static u32 kNumDistanceSensorReadingsToRecord = 50;

// ----------Camera Calibration----------
// Exposure setting for playpen
static u16 kExposure_ms                               = 3;

// Gain setting for playpen
static f32 kGain                                      = 2.f;

// Tolerance on the calculated camera focal length
static u32 kFocalLengthTolerance                      = 30;

// Tolerance on the calculated camera center
static u32 kCenterTolerance                           = 30;

// Tolerance on the camera's radial distortion
static f32 kRadialDistortionTolerance                 = 0.05f;

// Tolerance on the camera's tangential distortion
static f32 kTangentialDistortionTolerance             = 0.05f;

// Head angle at which we can see the entire calibration target
static f32 kHeadAngleToSeeTarget                      = MAX_HEAD_ANGLE;

// How long we should wait to see the calibration target after looking up at it
static u32 kTimeoutWaitingForTarget_ms                = 3000;

// How long we should wait for camera calibration to be computed
static u32 kTimeoutForComputingCalibration_ms         = 2000;

// Which calibration target we are using (see CameraCalibrator::CalibTargetTypes)
static u32 kPlaypenCalibTarget                        = 2; // 1 = INVERTED_BOX 2 = BLEACHERS (I don't feel like including cameraCalibrator.h here)

// How big the calibration target's markers are
static f32 kCalibMarkerSize_mm                        = 15;

// How big the calibration target's entire marker faces are
static f32 kCalibMarkerCubeSize_mm                    = 30;

// Which marker we should wait to be seeing before automatically starting camera calibration
static CustomObjectMarker kMarkerToTriggerCalibration = CustomObjectMarker::Triangles2;

// ----------Drive Forwards----------
// Distance to drive forwards to trigger the front cliff sensors
static f32 kDistanceToTriggerFrontCliffs_mm = 140;

//  Distance to drive forwards to trigger the back cliff sensors after trigger the front ones
static f32 kDistanceToTriggerBackCliffs_mm  = 80;

// Distance to drive forwards to have all cliff sensors on the ground after triggering the
// back cliff sensors
static f32 kDistanceToDriveOverCliff_mm     = 120;

// Speed at which to drive forwards towards cliff
static f32 kCliffSpeed_mmps                 = 50;

// Time to wait for a cliff event after getting a robot stopped event
static f32 kTimeToWaitForCliffEvent_ms      = CLIFF_EVENT_DELAY_MS + 100;

// ----------Tool Code----------
// Number of tool codes we should be seeing
static const u32 kNumToolCodes = 2;

// Allowed difference between expected and actual x pixel values of detected tool codes
static f32       kToolCodeDistThreshX_pix = 20.f;

// Allowed difference between expected and actual y pixel values of detected tool codes
static f32       kToolCodeDistThreshY_pix = 40.f;

// ----------Pickup Cube----------
// Allowed difference between expected and observed cube pose in x and y
static f32 kExpectedCubePoseDistThresh_mm       = 30;

// Allowed difference between expected and observed cube pose height
static f32 kExpectedCubePoseHeightThresh_mm     = 10;

// Allowed difference between expected and observed cube pose angle
static f32 kExpectedCubePoseAngleThresh_rad     = DEG_TO_RAD(10);

// How much the robot can rotate while backing up after putting the cube down
static f32 kMaxRobotAngleChangeDuringBackup_rad = DEG_TO_RAD(10);

}
}
}

#endif // __Cozmo_Basestation_PlaypenConfig__
