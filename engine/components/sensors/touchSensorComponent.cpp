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
#include "engine/components/battery/batteryComponent.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "osState/osState.h"
#include "webServerProcess/src/webService.h"

#include "util/logging/DAS.h"

#define LOG_CHANNEL "TouchSensor"

namespace Anki {
namespace Vector {
  
namespace {
  const std::string kLogDirectory = "touchSensor";
  
  const std::string kWebVizTouchSensorModuleName = "touch";
  
  // Baseline calibration constants:
  // -- maximum number of readings to wait for until we determine calibrated
  const int kBaselineMaxCalibReadings = 400;
  
  // -- number of "valid near baseline" to wait for until considered calibrated
  const int kBaselineMinReadingsCountForStability = 30;
  
  // -- coefficient for a IIR filter in fast accumulation mode
  const float kBaselineFilterCoeffFast = 0.01f;
  
  // -- coefficient for a IIR filter in slow accumulation mode
  const float kBaselineFilterCoeffSlow = 0.001f;
  
  // -- min no-touch cycles since last touch to reenable baseline accumulation
  const int kBaselineMinCountNoTouch = 100; // ~3 seconds of no-touch
  
  // maximum drift from the baseline the input reading signal to tolerate
  const int kAllowedDifferenceFromBaseline = 25;
  
  // number of ticks to stay in fast calibration when trying to recover
  // from a deviated baseline
  const int kFastCalibrationDuration = 50;
  
  // minimum number of TooLow readings before we enter fast calibration
  const int kMinTooLowReadingsForFastCalib = 50;
  
  // press-state represented as a bool, however we require a minimum
  // number of consecutive readings in order to confirm as true
  // note: this is a filtering measure to prevent false touches from
  // firing (they tend to fluctuate wildly, and are less like to trip this)
  const int kMinConsecReadingsToConfirmPressState = 4;
  
  // --- charger mode check constants
  
  // maximum number of ticks after a charger mode change was detected
  // after which we determine that no drastic change to the
  // measured baseline is needed
  const int kChargerModeCheckTimeout = 135;
  
  // number of ticks after enabling the charger mode check to wait before
  // taking a initial reading for accumulating into the proposal baseline
  const int kChargerModeCheckDelayAccumulation = 10;
  
  // minimum number of readings to be accumulated into the proposal
  // baseline for a post charger-mode change check
  const int kChargerModeCheckMinBaselineReadings = 50;
  
  // max allowed difference between proposal and current baseline touch
  const int kChargerModeCheckAllowedDifference = 25;
  
  // minimum required number of overly large difference between proposal
  // and current touch baseline to force the current baseline to be
  // overwritten
  const int kChargerModeCheckMinLargeDiffCount = 50;
  
  bool devSendTouchSensor = false;
  bool devTouchSensorPressed = false;
  #if ANKI_DEV_CHEATS
  void SetTouchSensor(ConsoleFunctionContextRef context)
  {
    devTouchSensorPressed = ConsoleArg_Get_Bool(context, "pressed");
    devSendTouchSensor = true;
  }
  CONSOLE_FUNC(SetTouchSensor, "Touch", bool pressed);
  
  CONSOLE_VAR(bool, kTestOnlyLoggingEnabled, "Touch", false);
  #endif
  
  const int kMaxPreTouchSamples = 100; // 100 ticks * 30ms = 3 seconds
  
  const int kMinPostTouchSamples = 100; // 100 * 30ms = 3 sec
  
  const int kMaxDevLogBufferSize = 2000;// 60 seconds / 30ms = 2000 samples
  
  const f32 kSendWebVizDataPeriod_sec = 0.5f;
  f32 _nextSendWebVizDataTime_sec = 0.0f;
  size_t _touchCountForWebviz = 0;
  Json::Value _toSendJson;
  
} // end anonymous namespace

void TouchBaselineCalibrator::UpdateBaseline(float reading, bool isPickedUp, bool isPressed)
{
  static float lastBaseline = 0.0f;
  
  if( isPickedUp ) {
    _numConsecNoTouch = 0;
    if(!IsCalibrated()) {
      _numStableBaselineReadings = 0;
      lastBaseline = 0.0f;
    }
    return;
  }
  
  // check if the incoming readings have settled within an acceptable
  // range from the baseline in order to be considered "calibrated"
  const bool isBaselineStable = fabsf(lastBaseline - _baseline) < 0.5;
  if(!IsCalibrated() && isBaselineStable) {
    ++_numStableBaselineReadings;
    if(_numStableBaselineReadings > kBaselineMinReadingsCountForStability) {
      LOG_INFO("TouchSensorComponent.UpdateBaseline.Calibrated", "%4.2f", _baseline);
      _isCalibrated = true;
    }
  } else if(!IsCalibrated() && !isBaselineStable) {
    _numStableBaselineReadings = 0;
    lastBaseline = _baseline;
  }
  
  if(!IsCalibrated()) {
    // take the first reading as the baseline to help converge faster
    // otherwise incorporate the reading with fast calibration
    if(_baseline < 0.0f) {
      _baseline = reading;
      lastBaseline = reading;
    } else {
      _baseline = kBaselineFilterCoeffFast*reading + (1.0f-kBaselineFilterCoeffFast)*_baseline;
    }
    
    // acts as a escape hatch to just quit trying to calibrate
    _numCalibrationReadings++;
    if(_numCalibrationReadings > kBaselineMaxCalibReadings) {
      PRINT_NAMED_WARNING("TouchSensorComponent.UpdateBaseline.CalibratedWithoutStability", "%4.2f", _baseline);
      _isCalibrated = true;
    }
  } else {
    // Sometimes when receiving light touches, they are:
    // 1. classified as NO TOUCH
    // 2. incorporated into the baseline accumulation
    // this results in a rising baseline while being touched
    // this `maybeTouch` guards against this happening
    const bool maybeTouch = (reading >= (_baseline + _maxAllowedOffsetFromBaseline));
    
    // require seeing a minimum number of untouched cycles before
    // continuing accumulation of values for baseline detect
    if(!isPressed && !maybeTouch) {
      _numConsecNoTouch++;
      if(_numConsecNoTouch >= kBaselineMinCountNoTouch) {
        DEV_ASSERT_MSG(_baseline > 0.0f, "TouchBaselineCalibrator.BaselineNotFastCalibrated", "");
        if( _durationForFastCalibration > 0 ) {
          _baseline = kBaselineFilterCoeffFast*reading + (1.0f-kBaselineFilterCoeffFast)*_baseline;
          _durationForFastCalibration--;
          if(_durationForFastCalibration == 0) {
            DASMSG(touch_baseline_fast_calibration_finished,
                   "touch_sensor.baseline_fast_calibration_finished",
                   "when the robot finishes fast calibration");
            DASMSG_SET(i1, std::round(_baseline), "baseline value after fast calibration");
            DASMSG_SEND();
          }
        } else {
          _baseline = kBaselineFilterCoeffSlow*reading + (1.0f-kBaselineFilterCoeffSlow)*_baseline;
        }
        _numConsecNoTouch = kBaselineMinCountNoTouch;
      }
    } else {
      _numConsecNoTouch = 0;
    }
  }
  
  // charger mode check handles the case where the baseline of the touch
  // signal shifts by a large amount after switching from off-to-on or
  // vice versa.
  // In this mode, we:
  // + initialize and accumulate proposal baseline, separate from the current baseline
  // + compare the proposal and current baselines
  // + if the difference is too large for too long, then we overwrite
  //   the current baseline with the proposal. Otherwise, time out of
  //   the charger mode check since it is within bounds.
  if(_chargerModeChanged) {
    if(_numPostChargerModeReadings < kChargerModeCheckDelayAccumulation) {
      // built-in delay for accumulation in to the proposal baseline
      // this is done to avoid fluctuations in the signal soon after
      // being taken off or onto the charger
      _numPostChargerModeReadings++;
    } else if(_numPostChargerModeReadings == kChargerModeCheckDelayAccumulation) {
      // first possible reading for the post charger-change for new baseline
      _postChargerModeChangeBaseline = reading;
      _numPostChargerModeReadings++;
    } else {
      // accumulate quickly because we have a finite
      // number of readings to come up with a proposal baseline
      _postChargerModeChangeBaseline = kBaselineFilterCoeffFast*reading + (1.0f-kBaselineFilterCoeffFast)*_postChargerModeChangeBaseline;
      _numPostChargerModeReadings++;
      if(_numPostChargerModeReadings > kChargerModeCheckTimeout) {
        // did not receive enough evidence that we should overwrite the current baseline
        LOG_INFO("TouchBaselineCalibrator.UpdateBaseline.NoBaselineChangeDueToChargerModeSwitch","%d",_numTicksLargeDifferenceInBaselines);
        DASMSG(touch_charger_mode_check_no_baseline_change,
               "touch_sensor.charger_mode_check.no_baseline_change",
               "send when robot determines no change in the baseline needed");
        DASMSG_SET(i1, (int)_numTicksLargeDifferenceInBaselines, "number of ticks of large deviation between proposal/current baseline");
        DASMSG_SEND();
        _chargerModeChanged = false;
        ResetBaselineMonitoring();
      } else if (_numPostChargerModeReadings > kChargerModeCheckMinBaselineReadings) {
        _numTicksLargeDifferenceInBaselines += fabsf(_baseline - _postChargerModeChangeBaseline) > kChargerModeCheckAllowedDifference;
        if(_numTicksLargeDifferenceInBaselines > kChargerModeCheckMinLargeDiffCount) {
          // received enough evidence since initializing the proposal baseline to overwrite
          LOG_INFO("TouchBaselineCalibrator.UpdateBaseline.BaselineChangedDueToChargerModeSwitch","%4.2f, %4.2f", _baseline, _postChargerModeChangeBaseline);
          DASMSG(touch_charger_mode_check_baseline_changed,
                 "touch_sensor.charger_mode_check.baseline_changed",
                 "send when robot determines baseline should be changed");
          DASMSG_SET(i1, std::round(_baseline), "old baseline value");
          DASMSG_SET(i2, std::round(_postChargerModeChangeBaseline), "new baseline value");
          DASMSG_SEND();
          _baseline = _postChargerModeChangeBaseline;
          _chargerModeChanged = false;
          ResetBaselineMonitoring();
        }
      }
    }
  } else {
    // check for the fast calibration recovery mode:
    // if the input signal is lower than the baseline by enough units for
    // enough consecutive readings, then we enter fast calibration mode
    // for a preset number of ticks ( to speed up the recovery process )
    const bool readingTooLow = (_baseline - reading) > kAllowedDifferenceFromBaseline;
    if(readingTooLow && _durationForFastCalibration==0) {
      _numTicksReadingsTooLowFromBaseline++;
      if(_numTicksReadingsTooLowFromBaseline > kMinTooLowReadingsForFastCalib) {
        LOG_INFO("TouchBaselineCalibrator.UpdateBaseline.FastCalibrationDueToLowSignal","%4.2f",_baseline);
        DASMSG(touch_baseline_reading_difference_too_low,
               "touch_sensor.baseline_reading_difference_monitor_too_low",
               "send when robot detects the current signal input is deviating below the baseline too much for too long");
        DASMSG_SET(i1, std::round(_baseline), "current baseline value");
        DASMSG_SEND();
        _durationForFastCalibration = kFastCalibrationDuration;
        ResetBaselineMonitoring();
      }
    } else {
      _numTicksReadingsTooLowFromBaseline = 0;
    }
  }
}

// values chosen manually after inspecting some robots --- parameter sweep not necessary
const TouchSensorComponent::FilterParameters TouchSensorComponent::_kFilterParamsForProd =
{
  .boxFilterSize                      = 5,
  .dynamicThreshGap                   = 20,
  .dynamicThreshDetectFollowOffset    = 10,
  .dynamicThreshUndetectFollowOffset  = 19,
  .dynamicThreshMaximumDetectOffset   = 100,
  .dynamicThreshMininumUndetectOffset = 45,
  .minConsecCountToUpdateThresholds   = 6,
};

TouchSensorComponent::TouchSensorComponent() 
: ISensorComponent(kLogDirectory)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::TouchSensor)
, _filterParams(_kFilterParamsForProd)
, _lastRawTouchValue(0)
, _baselineCalibrator(_filterParams.dynamicThreshMininumUndetectOffset)
, _boxFilterTouch(_filterParams.boxFilterSize)
, _detectOffsetFromBase(_filterParams.dynamicThreshMininumUndetectOffset+
                        _filterParams.dynamicThreshGap)
, _undetectOffsetFromBase(_filterParams.dynamicThreshMininumUndetectOffset)
, _isPressed(false)
, _countAboveDetectLevel(0)
, _countBelowUndetectLevel(0)
, _touchPressTime(std::numeric_limits<float>::max())
, _counterPressState(0)
, _confirmedPressState(false)
, _devLogMode(PreTouch)
, _devLogBuffer()
, _devLogBufferPreTouch()
, _devLogBufferSaved()
, _devLogSampleCount(0)
{
  static const std::set<u32> exceptionESNWithPVThardware = {
    0x00e10017,
    0x00e10074,
    0x00e10034,
    0x00e10057,
    0x00e1006d,
    0x00e10008,
    0x00e1005c,
    0x00e10052, // data set robot
    0x00e10062,
    0x00e1001f,
    0x00e10032,
    0x00e10064,
    0x00e1006c,
    0x00e10069,
    0x00e10014,
    0x00e10009,
    0x00e1006e,
    0x00e1001d,
  };
  
  // backwards compatibility check for robot hardware versions
  auto* osstate = OSState::getInstance();
  u32 esn = osstate->GetSerialNumber();
  const bool isValidESN = !(esn >= 0x00e00000 && esn <= 0x00e1ffff);
  const bool isExceptionESN = exceptionESNWithPVThardware.find(esn)!=exceptionESNWithPVThardware.end();
  _enabled = isValidESN || isExceptionESN;
  
  // send up the OS version (useful to determine if corresponding required syscon changes are present)
  int major, minor, incremental, build;
  osstate->GetOSBuildVersion(major, minor, incremental, build);
  std::stringstream osv;
  osv << major << "." << minor << "." << incremental << "." << build;
  
  // set once
  _toSendJson["esn"]        = osstate->GetSerialNumberAsString();
  _toSendJson["osVersion"]  = osv.str();
  
  // set multiple times
  _toSendJson["enabled"]    = _enabled ? "true" : "false";
  _toSendJson["count"]      = (int)_touchCountForWebviz;
  _toSendJson["calibrated"] = _baselineCalibrator.IsCalibrated() ? "yes" : "no";
}
  
TouchSensorComponent::~TouchSensorComponent()
{
  DevLogTouchSensorData();
}
  
void TouchSensorComponent::InitDependent(Robot* robot, const RobotCompMap& dependentComps)
{
  InitBase(robot);
  
  // webviz subscription to control the touch sensor config
  if (ANKI_DEV_CHEATS) {
    const auto* context = robot->GetContext();
    if (context != nullptr) {
      auto* webService = context->GetWebService();
      if (webService != nullptr) {
        // set up handler for incoming payload from WebViz
        auto onData = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
          if(data.isMember("enabled")) {
            const std::string& stateStr = JsonTools::ParseString(data, "enabled", "TouchSensorComponent.WebVizJsonCallback");
            _enabled = stateStr.compare("true")==0;
            // recalibrate and re-init the boxfilter
            _baselineCalibrator.ResetCalibration();
            _boxFilterTouch = BoxFilter(_filterParams.boxFilterSize);
          }

          if(data.isMember("resetCount")) {
            _touchCountForWebviz = 0;
          }
          
          // ack the mode change
          const auto* context = _robot->GetContext();
          if (context != nullptr) {
            auto* webService = context->GetWebService();
            if (webService != nullptr) {
              _toSendJson["enabled"] = _enabled ? "true" : "false";
              _toSendJson["count"] = (int)_touchCountForWebviz;
              webService->SendToWebViz(kWebVizTouchSensorModuleName, _toSendJson);
            }
          }
        };
        _signalHandles.emplace_back(webService->OnWebVizData(kWebVizTouchSensorModuleName).ScopedSubscribe(onData));
      }
    }
  }
}
  
void TouchSensorComponent::DevLogTouchSensorData() const
{
  if(ANKI_DEV_CHEATS) {
    std::string path = _robot->GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory);
    Util::FileUtils::CreateDirectory(path);
    
    path += "/";
    path += "lastTouchDump.csv";
    
    std::stringstream output;
    for(auto& row : _devLogBufferSaved) {
      output << row.rawTouch << ",";
      output << row.boxTouch << ",";
      output << row.calibBaseline << ",";
      output << row.detOffsetFromBase << ",";
      output << row.undOffsetFromBase << ",";
      output << (int)row.isPressed;
      output << std::endl;
    }
    
    if(!Util::FileUtils::WriteFile(path, output.str())) {
      PRINT_NAMED_WARNING("TouchSensorComponent.FailedToWrite","%s", path.c_str());
    }
  }
}

void TouchSensorComponent::NotifyOfRobotStateInternal(const RobotState& msg)
{
  // send the information for webviz once
  const auto* context = _robot->GetContext();
  if(context != nullptr) {
    auto* webService = context->GetWebService();
    if(webService != nullptr && webService->IsWebVizClientSubscribed(kWebVizTouchSensorModuleName)) {
      const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if (now_sec > _nextSendWebVizDataTime_sec) {
        _toSendJson["enabled"] = _enabled ? "true" : "false";
        _toSendJson["count"] = (int)_touchCountForWebviz;
        _toSendJson["calibrated"] = _baselineCalibrator.IsCalibrated() ? "yes" : "no";
        webService->SendToWebViz(kWebVizTouchSensorModuleName, _toSendJson);
        _nextSendWebVizDataTime_sec = now_sec + kSendWebVizDataPeriod_sec;
      }
    }
  }

  
  // in the process of going from DVT to PVT, the touch sensor hardware changed enough to
  // require a new scheme of sampling data from the sensor itself. As a result, the DVT
  // sensors are no longer supported. To guard against undefined behavior, we disable the
  // sensor in the robotics layer, if we detect here that the robot is not PVT hardware.
  //
  // DVT hardware => disabled
  // PVT hardware => enabled
  if(!_enabled) {
    return;
  }
  
  if(FACTORY_TEST &&_dataToRecord != nullptr)
  {
    _dataToRecord->data.push_back(msg.backpackTouchSensorRaw);
  }
  
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastRawTouchValue = msg.backpackTouchSensorRaw;
  float boxFiltVal = _boxFilterTouch.ApplyFilter(msg.backpackTouchSensorRaw);
  const bool isPickedUp = (msg.status & (uint32_t)RobotStatusFlag::IS_PICKED_UP) != 0;
  bool wasOnCharger = _isOnCharger;
  _isOnCharger = (msg.status & (uint32_t)RobotStatusFlag::IS_ON_CHARGER) != 0;
  bool lastPressed = _isPressed; // after this tick, state may change
  
  // activate the charger mode check if:
  // + we detect a change between the charger modes (off and on)
  // + we aren't already running the charger mode check
  // + we are not fast calibrating currently (potentially may cancel out drift)
  if(wasOnCharger != _isOnCharger &&
     !_baselineCalibrator.IsChargerModeCheckRunning() &&
     !_baselineCalibrator.IsInFastCalibrationMode()) {
    LOG_INFO("TouchSensorComponent.NotifyOfRobotStateInternal.BaselineChargerModeCheckActivated","");
    DASMSG(touch_charger_mode_check_activate,
           "touch_sensor.activate_charger_mode_check",
           "send when robot is checking the baseline because of a charger state change");
    DASMSG_SET(i1, (int)_isOnCharger, "whether the robot is on the charger or not");
    DASMSG_SEND();
    _baselineCalibrator.ActivateChargerModeCheck();
  }
  
  if( devSendTouchSensor ) {
    _robot->Broadcast(
      ExternalInterface::MessageEngineToGame(
        ExternalInterface::TouchButtonEvent(devTouchSensorPressed)));
    devSendTouchSensor = false;
    if(devTouchSensorPressed) {
      _touchPressTime = now;
    }
  }
  
  if( !_baselineCalibrator.IsCalibrated() ) {
    // note: treat isPressed as false, because we cannot detect touch while uncalibrated
    _baselineCalibrator.UpdateBaseline(boxFiltVal, isPickedUp, false);
  } else {
    bool lastPressed = _isPressed;
    if(!_baselineCalibrator.IsChargerModeCheckRunning()) {
      _isPressed = ProcessWithDynamicThreshold(std::floor(_baselineCalibrator.GetBaseline()),
                                               std::round(boxFiltVal));
    } else {
      // while monitoring the baseline after a charger-mode change
      // disable touch detections (since we cannot trust it yet)
      _isPressed = false;
    }
    
    // check to confirm the press state by counting the number of
    // consecutive detections from _isPressed
    bool lastConfirmedPressState = _confirmedPressState;
    if(lastPressed != _isPressed) {
      _counterPressState = 0;
    } else {
      if(_counterPressState < kMinConsecReadingsToConfirmPressState) {
        ++_counterPressState;
      } else {
        _confirmedPressState = _isPressed;
      }
    }
    
    // message any change in the confirmed press state
    if(lastConfirmedPressState != _confirmedPressState) {
      _robot->Broadcast(
        ExternalInterface::MessageEngineToGame(
          ExternalInterface::TouchButtonEvent(_confirmedPressState)));
      if(_confirmedPressState) {
        _touchPressTime = now;
        _touchCountForWebviz++;
      }
    }
    
#if ANKI_DEV_CHEATS
    if(kTestOnlyLoggingEnabled) {
      static FILE* fp = nullptr;
      if(fp==nullptr) {
        fp = fopen("/data/misc/touch.csv","w+");
      }
      fprintf(fp, "%d,%d,%d,%d,%d\n",
              _lastRawTouchValue,
              _isPressed,
              (int)_baselineCalibrator.GetBaseline(),
              _confirmedPressState,
              _baselineCalibrator.IsChargerModeCheckRunning());
    }
#endif    
    
    DEV_ASSERT( _baselineCalibrator.IsCalibrated(), "TouchSensorComponent.ClassifyingBeforeCalibration");
    // note: the baseline calibrator uses the raw touch detection, instead of the confirmed
    //  one instead, because we prefer it to be consevative in the values that it will accumulate
    _baselineCalibrator.UpdateBaseline(boxFiltVal, isPickedUp, _isPressed);
  }
  
  // dev-only logging code for touch sensor debugging
  //
  // note:
  // a touch event is defined as occuring iff:
  // + on the current tick, the pressed state is true
  // + on the previous tick, the pressed state is false
  //
  // The rules for logging the touch data are:
  // + M samples are recorded before a touch event
  // + N samples are recorded after a touch event
  // + if we get a touch event before we finish collecting
  //    N samples (post-touch), then we keep collecting
  //    until we can get N consecutive samples
  //
  // M = kMaxPreTouchSamples
  // N = kMinPostTouchSamples
  if(ANKI_DEV_CHEATS)
  {
    DevLogTouchRow entry
    {
      _lastRawTouchValue,
      boxFiltVal,
      (int)std::floor(_baselineCalibrator.GetBaseline()),
      _detectOffsetFromBase,
      _undetectOffsetFromBase,
      _isPressed
    };
    
    // always logging pretouch event, only copied when touch event occurs
    _devLogBufferPreTouch.push_back(entry);
    if(_devLogBufferPreTouch.size() > kMaxPreTouchSamples) {
      _devLogBufferPreTouch.pop_front();
    }

    const bool isTouchEvent = !lastPressed && _isPressed;
    if(isTouchEvent) {
      if(_devLogMode==PreTouch) {
        // seed the staging buffer with the pretouch data
        // if we are in PostTouch, then no copy is made since
        // we are already logging to the staging buffer
        _devLogBuffer = _devLogBufferPreTouch;
      }
      _devLogMode = PostTouch;
      // if we receive a TouchEvent while we are already
      // logging in PostTouch mode, then we reset the counter
      _devLogSampleCount = 0;
    }

    if(_devLogMode==PostTouch) {
      _devLogBuffer.push_back(entry);
      _devLogSampleCount++;
      
      // on logging enough consecutive PostTouch samples
      if(_devLogSampleCount > kMinPostTouchSamples || _devLogBuffer.size() > kMaxDevLogBufferSize) {
        _devLogBufferSaved = std::move(_devLogBuffer);
        _devLogBuffer.clear();
        _devLogMode = PreTouch;
        _devLogSampleCount = 0;
      }
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
  bool isPressed = _isPressed;
  
  // if input touch is within deadband for detect/undetect
  // then there will be no change in these values
  const int detectLevel = baseline + _detectOffsetFromBase;
  const int undetectLevel = baseline + _undetectOffsetFromBase;
  
  DEV_ASSERT( detectLevel >= undetectLevel, "DetectLevelLowerThanUndetectLevel");
  
  if (input > detectLevel) {
    isPressed = true;
    // monotonically increasing
    if ((input-detectLevel) > _filterParams.dynamicThreshDetectFollowOffset) {
      _countAboveDetectLevel++;
      _countBelowUndetectLevel = 0;
      if(_countAboveDetectLevel > _filterParams.minConsecCountToUpdateThresholds) {
        const auto candidateDetectOffset = input - _filterParams.dynamicThreshDetectFollowOffset - baseline;
        _detectOffsetFromBase = MIN(_filterParams.dynamicThreshMaximumDetectOffset, candidateDetectOffset);
        // detect and undetect levels move in lockstep
        _undetectOffsetFromBase = _detectOffsetFromBase - _filterParams.dynamicThreshGap;
        _countAboveDetectLevel = 0;
      }
    } else {
      // tracks count of *consecutive* readings which are
      // above or below the respective detect or undetect thresholds
      _countAboveDetectLevel = 0;
      _countBelowUndetectLevel = 0;
    }
  } else if (input < undetectLevel) {
    isPressed = false;
    // monotonically decreasing
    if ((undetectLevel-input) > _filterParams.dynamicThreshUndetectFollowOffset) {
      _countBelowUndetectLevel++;
      _countAboveDetectLevel = 0;
      if(_countBelowUndetectLevel > _filterParams.minConsecCountToUpdateThresholds) {
        auto candidateUndetectOffset = input + _filterParams.dynamicThreshUndetectFollowOffset - baseline;
        _undetectOffsetFromBase = MAX(_filterParams.dynamicThreshMininumUndetectOffset, candidateUndetectOffset);
        // detect and undetect levels move in lockstep
        _detectOffsetFromBase = _undetectOffsetFromBase + _filterParams.dynamicThreshGap;
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
  
  return isPressed;
}

bool TouchSensorComponent::GetIsPressed() const
{
  return _confirmedPressState;
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
