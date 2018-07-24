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

#include "coretech/common/shared/types.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/types/customObjectMarkers.h"
#include "clad/types/robotStatusAndActions.h"

#include "util/console/consoleVariable.h"

namespace Anki {
namespace Cozmo {
namespace PlaypenConfig {

// ----------General----------
// Whether or not to disconnect from robot at end of tests
static bool kDisconnectAtEnd      = false;

// Whether or not to write to robot storage
static bool kWriteToStorage       = true;

// Whether or not to ignore any test failures
// The playpen behaviors should be written in such a way so that they can continue running
// even after something has gone wrong (basically no branching)
static bool kIgnoreFailures       = false;

// Default timeout to force a playpen behavior to end
static f32  kDefaultTimeout_ms    = 20000;

// How long to display the playpen result on Cozmo's face
static f32  kTimeToDisplayResultOnFace_ms = 10000;

// Whether or not to skip checking we have heard from an active object
static bool kSkipActiveObjectCheck        = false;
 
// Whether or not to need touch to start playpen
static bool kUseTouchToStart              = false;

// How long the robot has to be touched until playpen starts
static u32  kTouchDurationToStart_ms      = 1000;

// Whether or not to need a button press to start 
static bool kUseButtonToStart             = true;

// How long to record touch sensor data
static u32  kDurationOfTouchToRecord_ms   = 0;

// Whether or not to check for cloud cert
static bool kCheckForCert                 = false;

// Path of cloud cert
static std::string kCertPath              = "/factory/cloud/something.pem";

// The cloud cert should be at least this large
static ssize_t     kMinCertSize_bytes     = 1000;

// Path to data directory
static std::string kDataDirPath           = "/data/data/com.anki.victor";

// If data dir is larger than this size, it is deleted
static ssize_t     kMaxDataDirSize_bytes  = 200000000;

// Threshold on the difference between max and min filtered touch sensor
// values during playpen
// NOTE: This value can be overridden by value stored in EMR
static f32 kMaxMinTouchSensorFiltDiff = 11.f;

// Max allowed standard deviation of filtered touch sensor values
// during playpen
// NOTE: This value can be overridden by value stored in EMR
static f32 kTouchSensorFiltStdDevThresh = 1.8f;
 
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

// Minimum expected raw touch sensor value
static u16       kMinExpectedTouchValue= 400;

// Maximum expected raw touch sensor value
static u16       kMaxExpectedTouchValue= 800;

// ----------Motor Calibration----------
// Timeout to wait for motor calibration to complete
static u32 kMotorCalibrationTimeout_ms = 4000;

// ----------Drift Check----------
// Head angle at which to check for drift
static f32 kHeadAngleForDriftCheck     = DEG_TO_RAD(0.f);

// How long to check drift for
static u32 kIMUDriftDetectPeriod_ms    = 2000;

// How much our heading can change over kIMUDriftDetectPeriod_ms
static f32 kIMUDriftAngleThreshDeg     = 0.2f;

// ----------Distance Sensor----------
// Number of distance sensor readings to record
static u32 kNumDistanceSensorReadingsToRecord       = 1;

// Threshold on calculated distance to distance sensor target (using detected marker)
// +/- this from the expected distance to the object/marker defined in the distance sensor
// behavior json file
static f32 kVisualDistanceToDistanceSensorObjectThresh_mm = 30;

// Threshold on the raw distance sensor reading from the expected distance defined in the
// distance sensor behavior json file
static f32 kDistanceSensorReadingThresh_mm = 20;

// Bias adjustment for raw distance sensor reading when comparing to visual distance
static f32 kDistanceSensorBiasAdjustment_mm = 30;

// ----------Camera Calibration----------
// Exposure setting for playpen
static u16 kExposure_ms                               = 31;

// Gain setting for playpen
static f32 kGain                                      = 1.f;

// Tolerance on the calculated camera focal length
static u32 kFocalLengthTolerance                      = 30;

// Tolerance on the calculated camera center
static u32 kCenterTolerance                           = 50;

// Tolerance on the camera's radial distortion
static f32 kRadialDistortionTolerance                 = 0.15f;

// Tolerance on the camera's tangential distortion
static f32 kTangentialDistortionTolerance             = 0.05f;

// Head angle at which we can see the entire calibration target
static f32 kHeadAngleToSeeTarget_rad                  = DEG_TO_RAD(42.f);

// How long we should wait to see the calibration target after looking up at it
static u32 kTimeoutWaitingForTarget_ms                = 5000;

// How long we should wait for camera calibration to be computed
static u32 kTimeoutForComputingCalibration_ms         = 5000;

// Which calibration target we are using (see CameraCalibrator::CalibTargetTypes)
static u32 kPlaypenCalibTarget                        = 2; // 1 = INVERTED_BOX 2 = QBERT (I don't feel like including cameraCalibrator.h here)

// How big the calibration target's markers are
static f32 kCalibMarkerSize_mm                        = 14;

// How big the calibration target's entire marker faces are
static f32 kCalibMarkerCubeSize_mm                    = 20;

// Which marker we should wait to be seeing before automatically starting camera calibration
static CustomObjectMarker kMarkerToTriggerCalibration = CustomObjectMarker::Triangles2;

// ----------Drive Forwards----------
// Distance to drive forwards to trigger the front cliff sensors
static f32 kDistanceToTriggerFrontCliffs_mm = 140;

//  Distance to drive forwards to trigger the back cliff sensors after trigger the front ones
static f32 kDistanceToTriggerBackCliffs_mm  = 80;

// Distance to drive forwards to have all cliff sensors on the ground after triggering the
// back cliff sensors
static f32 kDistanceToDriveOverCliff_mm     = 50;

// Speed at which to drive forwards towards cliff
static f32 kCliffSpeed_mmps                 = 60;

// Time to wait for a cliff event after getting a robot stopped event
static f32 kTimeToWaitForCliffEvent_ms      = CLIFF_EVENT_DELAY_MS + 100;

// Set threshold for cliff detection while the Drive Forwards behavior is running
static u16 kCliffSensorThreshold            = 180;

// ----------Sound Check----------
// Head angle at which we should play the sound for both speaker and mic check
static f32 kHeadAngleToPlaySound       = DEG_TO_RAD(2.f);

// Volume to play sound
static f32 kSoundVolume                = 1.f;

// How long to record audio for when we start playing the sound
static u32 kDurationOfAudioToRecord_ms = 2000;

// The expected frequency the mics should pickup when the sound is played
static u32 kFFTExpectedFreq_hz         = 1024;

// The allowed +/- deviation of the FFT frequency from the expected value
static u32 kFFTFreqTolerance_hz        = 20;

// ----------Pickup Cube----------
// Expected cube pose x relative to playpen starting pose
static f32 kExpectedCubePoseX_mm                = 420;

// Expected cube pose y relative to playpen starting pose
static f32 kExpectedCubePoseY_mm                = 0;

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
