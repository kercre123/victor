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
#include "util/console/consoleInterface.h"
#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirectory = "touchSensor";

  // CONSTANTS
  const float kMaxTouchIntensityInvalid = 800;      
  
  const int kMinCountNoTouchForAccumulation = 1000;
  

  const int kBaselineMinCalibReadings = 90;
  
  const float kBaselineFilterCoeff = 0.05f;
  
  const float kBaselineInitial = 500.0f;
  
  const int kBoxFilterSize = 8;
  
  // constant size difference between detect and undetect levels
  const int kDynamicThreshGap = 9;
  
  // the offset from the input reading that detect level is set to
  const int kDynamicThreshDetectFollowOffset = 7;
  
  // the offset from the input reading that the undetect level is set to
  const int kDynamicThreshUndetectFollowOffset = 7;
  
  // the highest reachable detect level offset from baseline
  const int kDynamicThreshMininumUndetectOffset = 13;
  
  // the lowest reachable undetect level offset from baseline
  const int kDynamicThreshMaximumDetectOffset = 50;
  
  // minimum number of consecutive readings that are below/above detect level
  // before we update the thresholds dynamically
  // note: this counters the impact of noise on the dynamic thresholds
  const int kMinConsecCountToUpdateThresholds = 8;
  
  bool devSendTouchSensor = false;
  bool devTouchSensorPressed = false;
  #if ANKI_DEV_CHEATS
  void SetTouchSensor(ConsoleFunctionContextRef context)
  {
    devTouchSensorPressed = ConsoleArg_Get_Bool(context, "pressed");
    devSendTouchSensor = true;
  }
  CONSOLE_FUNC(SetTouchSensor, "Touch", bool pressed);
  #endif
} // end anonymous namespace

TouchSensorComponent::TouchSensorComponent() 
: ISensorComponent(kLogDirectory)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::TouchSensor)
, _lastRawTouchValue(0)
, _noContactCounter(0)
, _baselineTouch(kBaselineInitial)
, _boxFilterTouch(kBoxFilterSize)
, _detectOffsetFromBase(kDynamicThreshMininumUndetectOffset+kDynamicThreshGap)
, _undetectOffsetFromBase(kDynamicThreshMininumUndetectOffset)
, _isPressed(false)
, _numConsecCalibReadings(0)
, _countAboveDetectLevel(0)
, _countBelowUndetectLevel(0)
, _touchPressTime(std::numeric_limits<float>::max())
{
}

void TouchSensorComponent::UpdateInternal(const RobotState& msg)
{
  if(FACTORY_TEST &&_dataToRecord != nullptr)
  {
    _dataToRecord->data.push_back(msg.backpackTouchSensorRaw);
  }

  // sometimes spurious values that are absurdly high come through the sensor
  if(msg.backpackTouchSensorRaw > kMaxTouchIntensityInvalid) {
    return;
  }
  
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  _lastRawTouchValue = msg.backpackTouchSensorRaw;
  
  int boxFiltVal = _boxFilterTouch.ApplyFilter(_lastRawTouchValue);
  
  const bool isPickedUp = (msg.status & (uint32_t)RobotStatusFlag::IS_PICKED_UP) != 0;
  
  if( devSendTouchSensor ) {
    _robot->Broadcast(
      ExternalInterface::MessageEngineToGame(
        ExternalInterface::TouchButtonEvent(devTouchSensorPressed)));
    devSendTouchSensor = false;
    if(devTouchSensorPressed) {
      _touchPressTime = now;
    }
  }
  
  if( !IsCalibrated() ) {
    // note: do not detect touch instances during calibration
    if(!isPickedUp) {
      _baselineTouch = kBaselineFilterCoeff*boxFiltVal + (1.0f-kBaselineFilterCoeff)*_baselineTouch;
      _numConsecCalibReadings++;
      if(IsCalibrated()) {
        PRINT_NAMED_INFO("TouchSensorComponent","Calibrated");;
      }
    } else {
      _numConsecCalibReadings = 0;
    }
  } else {
    // handle checks for touch input, when calibrated
    if( ProcessWithDynamicThreshold(_baselineTouch, boxFiltVal) ) {
      _robot->Broadcast(
        ExternalInterface::MessageEngineToGame(
          ExternalInterface::TouchButtonEvent(GetIsPressed())));
      if(GetIsPressed()) {
        _touchPressTime = now;
      }
    }
    // require seeing a minimum number of untouched cycles before
    // continuing accumulation of values for baseline detect
    DEV_ASSERT( IsCalibrated(), "TouchSensorComponent.ClassifyingBeforeCalibration");
    if(!GetIsPressed() && !isPickedUp) {
      _noContactCounter++;
      if(_noContactCounter > kMinCountNoTouchForAccumulation) {
        _baselineTouch = kBaselineFilterCoeff*boxFiltVal + (1.0f-kBaselineFilterCoeff)*_baselineTouch;
      }
    } else {
      // if we are picked up OR we are being touched
      _noContactCounter = 0;
    }
  }
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

void TouchSensorComponent::StartRecordingData(TouchSensorValues* data)
{
  _dataToRecord = data;
}
  
bool TouchSensorComponent::ProcessWithDynamicThreshold(const int baseline, const int input)
{
  // if input touch is within deadband for detect/undetect
  // then there will be no change in these values
  // Additionally _isPressed state won't change
  bool lastPressed = _isPressed;
  
  const int detectLevel = baseline + _detectOffsetFromBase;
  const int undetectLevel = baseline + _undetectOffsetFromBase;
  
  DEV_ASSERT( detectLevel > undetectLevel, "DetectLevelLowerThanUndetectLevel");
  
  if (input > detectLevel) {
    _isPressed = true;
    // monotonically increasing
    if ((input-detectLevel) > kDynamicThreshDetectFollowOffset) {
      _countAboveDetectLevel++;
      _countBelowUndetectLevel = 0;
      if(_countAboveDetectLevel > kMinConsecCountToUpdateThresholds) {
        const auto candidateDetectOffset = input - kDynamicThreshDetectFollowOffset - baseline;
        _detectOffsetFromBase = MIN(kDynamicThreshMaximumDetectOffset, candidateDetectOffset);
        // detect and undetect levels move in lockstep
        _undetectOffsetFromBase = _detectOffsetFromBase - kDynamicThreshGap;
        _countAboveDetectLevel = 0;
      }
    } else {
      // tracks count of *consecutive* readings which are
      // above or below the respective detect or undetect thresholds
      _countAboveDetectLevel = 0;
      _countBelowUndetectLevel = 0;
    }
  } else if (input < undetectLevel) {
    _isPressed = false;
    // monotonically decreasing
    if ((undetectLevel-input) > kDynamicThreshUndetectFollowOffset) {
      _countBelowUndetectLevel++;
      _countAboveDetectLevel = 0;
      if(_countBelowUndetectLevel > kDynamicThreshDetectFollowOffset) {
        auto candidateUndetectOffset = input + kDynamicThreshUndetectFollowOffset - baseline;
        _undetectOffsetFromBase = MAX(kDynamicThreshMininumUndetectOffset, candidateUndetectOffset);
        // detect and undetect levels move in lockstep
        _detectOffsetFromBase = _undetectOffsetFromBase + kDynamicThreshGap;
        _countBelowUndetectLevel = 0;
      }
    } else {
      // tracks count of *consecutive* readings which are
      // above or below the respective detect or undetect thresholds
      _countAboveDetectLevel = 0;
      _countBelowUndetectLevel = 0;
    }
  } else {
    _countAboveDetectLevel = 0;
    _countBelowUndetectLevel = 0;
  }
  
  return lastPressed != _isPressed;
}
  
bool TouchSensorComponent::IsCalibrated() const {
  return _numConsecCalibReadings >= kBaselineMinCalibReadings;
}

bool TouchSensorComponent::GetIsPressed() const
{
  return _isPressed;
}

int TouchSensorComponent::GetDetectLevelOffset() const
{
  return _detectOffsetFromBase;
}

int TouchSensorComponent::GetUndetectLevelOffset() const
{
  return _undetectOffsetFromBase;
}
  
float TouchSensorComponent::GetTouchPressTime() const
{
  return _touchPressTime;
}
  
} // Cozmo namespace
} // Anki namespace
