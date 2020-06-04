/**
 * File: selfTestConfig.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Configuration file for self test
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_SelfTestConfig__
#define __Cozmo_Basestation_SelfTestConfig__

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/shared/types.h"

#include "util/console/consoleVariable.h"

#include "clad/types/robotStatusAndActions.h"

namespace Anki {
namespace Vector {
namespace SelfTestConfig {

// ----------General----------
// Whether or not to ignore any test failures
// The self test behaviors should be written in such a way so that they can continue running
// even after something has gone wrong (basically no branching)
static bool kIgnoreFailures       = false;

// Default timeout to force a self test behavior to end
static f32  kDefaultTimeout_ms    = 20000;

// How long to display the self test result on Vector's face
static f32  kTimeToDisplayResultOnFace_ms = 10000;

// Whether or not to skip checking we have heard from an active object
static bool kSkipActiveObjectCheck        = true;

// SSID we should try to scan and connect to for the radio check
static std::string kWifiSSID = "Bf#tm$5[hzs]QX0g";

// Password for above SSID
static std::string kWifiPwd = "srw1JWOnjq;$YlB,";

// ---------PickupChecks--------
// How long the robot needs to be held upside down
// before the cliff sensors are checked
static u16 kTimeToBeUpsideDown_ms = 2000;

// Z acceleration threshold for determining if the robot is upside down
static s16 kUpsideDownZAccel      = -7000;

// Cliff sensors must be reporting less than this value when the robot is upside down
static u16 kUpsideDownCliffValThresh  = 50;

// Once the robot has been turned upside down, if more than this amount of time passes
// the test will fail
static u16 kUpsideDownTimeout_ms = 5000;

// ----------InitChecks----------
// Minimum battery voltage the robot should have at the start and end of self test
static f32 kMinBatteryVoltage     = 3.6;

// Minimum charger voltage the robot should see when on the charger at the beginning of the self test
static f32 kMinChargerVoltage     = 4.f;

// Minimum expected raw touch sensor value
static u16 kMinExpectedTouchValue = 3000;

// Maximum expected raw touch sensor value
static u16 kMaxExpectedTouchValue = 7000;

// How long the touch sensor needs to be held
static u16 kTouchSensorDuration_sec = 5;

// Distance to drive backwards at the beginning of the self test to align properly on the charger
static u16 kDriveBackwardsDist_mm = 100;

// Speed at which to drive backwards while on the charger
static u16 kDriveBackwardsSpeed_mmps = 40;

// Time to spend driving backwards while on the charger
// Pose doesn't translate while on charger so drive is time based
static f32 kDriveBackwardsTime_sec = 1.f;

// Minimum cliff sensor value while on the charger
static u16 kMinCliffSensorOnChargerVal = 50;

// Max cliff sensor value while on the charger
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
static f32 kDistanceSensorReadingThresh_mm = 30;

// Bias adjustment for raw distance sensor reading when comparing to visual distance
static f32 kDistanceSensorBiasAdjustment_mm = -12;

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

// ----------Screen and Backpack----------
// Distance to drive off the charger
static u16 kDistToDriveOffCharger_mm = 10;

// Speed to drive off the charger
static u16 kDriveOffChargerSpeed_mmps = 60;

// Color to display on screen and backpack lights
static ColorRGBA kColorCheck = NamedColors::WHITE;

// Color of text displayed on screen
static ColorRGBA kTextColor = NamedColors::BLACK;

// Distance to drive to get back on the charger
static s16 kDistToDriveOnCharger_mm = -40;
}
}
}

#endif // __Cozmo_Basestation_SelfTestConfig__
