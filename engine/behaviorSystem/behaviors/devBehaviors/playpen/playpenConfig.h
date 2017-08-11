/**
 * File: playpenConfig.h
 *
 * Author: Al Chaussee
 * Created: 08/10/17
 *
 * Description:
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

static bool  kDisconnectAtEnd      = false;
static bool  kWriteToStorage       = false;
static bool  kIgnoreFailures       = true;
static f32   kDefaultTimeout_ms    = 20000;

// InitChecks
static bool      kCheckFirmwareVersion = true;
static f32       kMinBatteryVoltage    = 3.6;
static u32       kMinFirmwareVersion   = 10501;
static int       kMinHardwareVersion   = 3;
static u32       kMfgIDTimeout_ms      = 500;
static BodyColor kMinBodyColor         = BodyColor::WHITE_v15;

// Motor Calibration
static u32 kMotorCalibrationTimeout_ms = 4000;

// Drift Check
static f32 kHeadAngleToPlaySound    = DEG_TO_RAD(2.f);
static u32 kIMUDriftDetectPeriod_ms = 2000;
static f32 kIMUDriftAngleThreshDeg  = 0.2f;

// Camera Calibration
static u16 kExposure_ms                               = 3;
static f32 kGain                                      = 2.f;
static u32 kFocalLengthTolerance                      = 30;
static u32 kCenterTolerance                           = 30;
static f32 kRadialDistortionTolerance                 = 0.05f;
static f32 kTangentialDistortionTolerance             = 0.05f;
static f32 kHeadAngleToSeeTarget                      = MAX_HEAD_ANGLE;
static u32 kTimeoutWaitingForTarget_ms                = 10000;
static u32 kTimeoutForComputingCalibration_ms         = 2000;
static f32 kCalibMarkerSize_mm                        = 15;
static f32 kCalibMarkerCubeSize_mm                    = 30;
static CustomObjectMarker kMarkerToTriggerCalibration = CustomObjectMarker::Triangles2;

// Drive Forwards
static f32 kDistanceToTriggerFrontCliffs_mm = 90;
static f32 kDistanceToTriggerBackCliffs_mm  = 60;
static f32 kDistanceToDriveOverCliff_mm     = 30;
static f32 kTimeToWaitForCliffEvent_ms      = CLIFF_EVENT_DELAY_MS + 100;

// Pickup Cube
static f32 kExpectedCubePoseDistThresh_mm       = 30;
static f32 kExpectedCubePoseHeightThresh_mm     = 10;
static f32 kExpectedCubePoseAngleThresh_rad     = DEG_TO_RAD(10);
static f32 kMaxRobotAngleChangeDuringBackup_rad = DEG_TO_RAD(10);

}
}
}

#endif // __Cozmo_Basestation_PlaypenConfig__
