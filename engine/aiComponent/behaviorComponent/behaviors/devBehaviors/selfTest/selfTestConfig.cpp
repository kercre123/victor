/**
 * File: selfTestConfig.h
 *
 * Author: Al Chaussee
 * Created: 08/10/17
 *
 * Description: Configuration file for self test
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/selfTestConfig.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
namespace SelfTestConfig {

WRAP_EXTERN_CONSOLE_VAR(bool,  kIgnoreFailures,               "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(float, kDefaultTimeout_ms,            "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(float, kTimeToDisplayResultOnFace_ms, "SelfTest");

WRAP_EXTERN_CONSOLE_VAR(u16, kTimeToBeUpsideDown_ms, "SelfTest");

WRAP_EXTERN_CONSOLE_VAR(f32,  kMinBatteryVoltage,     "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(u16,  kMinExpectedTouchValue, "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(u16,  kMaxExpectedTouchValue, "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(u16,  kDriveBackwardsDist_mm, "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(u16,  kDriveBackwardsSpeed_mmps, "SelfTest");

WRAP_EXTERN_CONSOLE_VAR(u32, kMotorCalibrationTimeout_ms, "SelfTest");

WRAP_EXTERN_CONSOLE_VAR(f32, kHeadAngleForDriftCheck,     "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(f32, kHeadAngleToPlaySound,       "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(f32, kSoundVolume,                "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(u32, kDurationOfAudioToRecord_ms, "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(u32, kFFTExpectedFreq_hz,         "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(u32, kFFTFreqTolerance_hz,        "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(u32, kIMUDriftDetectPeriod_ms,    "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(f32, kIMUDriftAngleThreshDeg,     "SelfTest");

WRAP_EXTERN_CONSOLE_VAR(u32, kNumDistanceSensorReadingsToRecord,             "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(f32, kVisualDistanceToDistanceSensorObjectThresh_mm, "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(f32, kDistanceSensorReadingThresh_mm,                "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(f32, kDistanceSensorBiasAdjustment_mm,               "SelfTest");

WRAP_EXTERN_CONSOLE_VAR(f32, kDistanceToDriveForwards_mm, "SelfTest");
WRAP_EXTERN_CONSOLE_VAR(f32, kDriveSpeed_mmps,            "SelfTest");

WRAP_EXTERN_CONSOLE_VAR(u32, kChargerMarkerLastObservedTimeThresh_ms, "SelfTest");
                        
}
}
}
