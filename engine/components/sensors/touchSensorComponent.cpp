/**
 * File: touchSensorComponent.cpp
 *
 * Author: Arjun Menon
 * Created: 9/7/2017
 *
 * Description: Component for managing the capacitative touch sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirectory = "touchSensor";

  // XXX(agm): arbitrarily chosen constants for Victor-G
  const float kMaxTouchIntensityInvalid = 800;      
  
  const int kMinCountNoTouchForAccumulation = 1000; // = 30 sec / 30 ms rate

  // press debouncing constants
  const int kDebounceLimitPressLow = 3;
  const int kDebounceLimitPressHi = 4;

  // gesture classification constants
  const float kGestureMinTimeForHeld_s = 2.0f;
  const float kGestureMaxTimeWithoutTouch_s = 3.0f;
  const float kGestureMinTimeForSlowStroke_s = 0.48f;
  const int kGestureMaxBufferedStrokeCount = 3;
  const int kGestureMinNumSlowStrokes = 2;
  
  // baseline calibration constants
  // note: the fast calibration time is ~3 seconds
  //       = (25 samples) x (4 times) x (30ms / cycle)
  const int kBaselineNumUpdatesForFastCalib = 4;
  const int kBaselineNumSamplesPerUpdateForFastCalib = 25;
  // coefficients for lowpass filtering where:
  // 0 = static unchanging output (set once)
  // 1 = filter takes on latest input
  const float kBaselineFilterCoeffSlow = 0.0005f;
  const float kBaselineFilterCoeffFast = 0.2f;
  // the maximum number of standard deviations to allow sensor readings
  // during the baseline fast-calibration phase before restarting the phase
  const float kBaselineRestartMaxStdevFactor = 3.5f;
  
  // the number of standard deviations to consider sensor readings as "noise"
  const float kTouchDetectStdevFactor = 1.5f;

  // max allowable buffer standard deviation when we accumulate
  // into the filtered standard deviation. If input stdev is too
  // large, then assume there is touch and restart calibration
  const float kBaselineMaxAllowableBufferStdev = 15;

  const float kBaselineMaxAllowStdevFactorForLowMean = 1.5f;
} // end anonymous namespace

TouchSensorComponent::TouchSensorComponent(Robot& robot) 
: ISensorComponent(robot, kLogDirectory)
, _debouncer(kDebounceLimitPressLow, 
             kDebounceLimitPressHi)
, _gestureClassifier(kGestureMinTimeForHeld_s,
                     kGestureMaxTimeWithoutTouch_s,
                     kGestureMinTimeForSlowStroke_s,
                     kGestureMaxBufferedStrokeCount,
                     kGestureMinNumSlowStrokes)
, _baselineCalib(kBaselineNumUpdatesForFastCalib,
                 kBaselineNumSamplesPerUpdateForFastCalib,
                 kBaselineFilterCoeffSlow,
                 kBaselineFilterCoeffFast,
                 kBaselineRestartMaxStdevFactor,
                 kBaselineMaxAllowableBufferStdev,
                 kBaselineMaxAllowStdevFactorForLowMean)
, _touchDetectStdevFactor(kTouchDetectStdevFactor)
, _touchGesture(TouchGesture::NoTouch)
, _noContactCounter(0)
{
}

void TouchSensorComponent::UpdateInternal(const RobotState& msg)
{
  // sometimes spurious values that are absurdly high come through the sensor
  if(msg.backpackTouchSensorRaw > kMaxTouchIntensityInvalid) {
    return;
  }

  const bool isPickedUp = (msg.status & (uint32_t)RobotStatusFlag::IS_PICKED_UP) != 0;

  TouchGesture curGesture = _touchGesture;
  if( !_baselineCalib.IsCalibrated() ) {
    // note: do not detect touch instances during fast-calibration
    if(!isPickedUp) {
      _baselineCalib.UpdateCalibration(msg.backpackTouchSensorRaw);
    }
    curGesture = TouchGesture::NoTouch;
  } else {
    const auto normTouch = msg.backpackTouchSensorRaw-_baselineCalib.GetFilteredTouchMean();
    const bool isTouched = normTouch > 
                            (_touchDetectStdevFactor*_baselineCalib.GetFilteredTouchStdev());
    if( _debouncer.ProcessRawPress(isTouched) ) {
      const bool debouncedButtonState = _debouncer.GetDebouncedPress();
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(
                        ExternalInterface::TouchButtonEvent(debouncedButtonState)));
      if( debouncedButtonState ) {
        _gestureClassifier.AddTouchPressed();
      } else {
        _gestureClassifier.AddTouchReleased();
      }
    }
    curGesture = _gestureClassifier.CalcTouchGesture();

    // require seeing a minimum number of untouched cycles before
    // continuing accumulation of values for baseline detect
    if(!isTouched && !isPickedUp) {
      _noContactCounter++;
      if(_noContactCounter > kMinCountNoTouchForAccumulation) {
        _baselineCalib.UpdateCalibration(msg.backpackTouchSensorRaw);
      }
    } else {
      // if we are picked up OR we are being touched
      _noContactCounter = 0;
    }
  }

  if (curGesture != _touchGesture) {
    _touchGesture = curGesture;
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::TouchGestureEvent(_touchGesture)));
  }  

}

bool TouchSensorComponent::IsTouched() const
{
  return _debouncer.GetDebouncedPress();
}

std::string TouchSensorComponent::GetLogHeader()
{
  return std::string("timestamp_ms, touchIntensity");
}


std::string TouchSensorComponent::GetLogRow()
{
  std::string str;
  // XXX(agm): figure out what to log
  return str;
}

  
} // Cozmo namespace
} // Anki namespace
