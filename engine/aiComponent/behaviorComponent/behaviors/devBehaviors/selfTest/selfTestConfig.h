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
namespace SelfTestConfig {

// ----------General----------
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

// Threshold on the difference between max and min filtered touch sensor
// values during playpen
// NOTE: This value can be overridden by value stored in EMR
static f32 kMaxMinTouchSensorFiltDiff = 11.f;

// Max allowed standard deviation of filtered touch sensor values
// during playpen
// NOTE: This value can be overridden by value stored in EMR
static f32 kTouchSensorFiltStdDevThresh = 1.8f;

// ---------PickupChecks--------
static u16 kTimeToBeUpsideDown_ms = 2000;

// ----------InitChecks----------
// Minimum battery voltage the robot should have at the start and end of playpen
static f32 kMinBatteryVoltage     = 3.6;

// Minimum expected raw touch sensor value
static u16 kMinExpectedTouchValue = 3000;

// Maximum expected raw touch sensor value
static u16 kMaxExpectedTouchValue = 7000;

static u16 kDriveBackwardsDist_mm = 100;

static u16 kDriveBackwardsSpeed_mmps = 40;

static u16 kMinCliffSensorOnChargerVal = 50;

static u16 kMaxCliffSensorOnChargerVal = 1000;

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
static f32 kDistanceSensorBiasAdjustment_mm = 0;

// ----------Drive Forwards----------
// Distance to drive forwards to trigger the front cliff sensors
static f32 kDistanceToDriveForwards_mm = 120;

// Speed at which to drive forwards towards cliff
static f32 kDriveSpeed_mmps                 = 60;


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

// The charger marker's last observed time is allowed to be this old compared to the latest processed image
static u32 kChargerMarkerLastObservedTimeThresh_ms = 500;

}
}
}

#endif // __Cozmo_Basestation_PlaypenConfig__
