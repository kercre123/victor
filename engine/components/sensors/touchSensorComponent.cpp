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
#include "clad/externalInterface/messageEngineToGame.h"
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

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirectory = "touchSensor";
  
  const std::string kWebVizTouchSensorModuleName = "touch";

  // CONSTANTS
  const float kMaxTouchIntensityInvalid = 800;      
  
  const int kMinCountNoTouchForAccumulation = 100; // ~3 seconds of no-touch
  

  const int kBaselineMinCalibReadings = 90;
  
  const float kBaselineFilterCoeff = 0.05f;
  
  const float kBaselineInitial = 500.0f;
  
  bool devSendTouchSensor = false;
  bool devTouchSensorPressed = false;
  bool devEnableTouchSensor = true;
  #if ANKI_DEV_CHEATS
  void SetTouchSensor(ConsoleFunctionContextRef context)
  {
    devTouchSensorPressed = ConsoleArg_Get_Bool(context, "pressed");
    devSendTouchSensor = true;
  }
  CONSOLE_FUNC(SetTouchSensor, "Touch", bool pressed);
  
  void EnableTouchSensing(ConsoleFunctionContextRef context)
  {
    devEnableTouchSensor = ConsoleArg_Get_Bool(context, "enable");
  }
  CONSOLE_FUNC(EnableTouchSensing, "Touch", bool enable);
  #endif
  
  const int kMaxPreTouchSamples = 100; // 100 ticks * 30ms = 3 seconds
  
  const int kMinPostTouchSamples = 100; // 100 * 30ms = 3 sec
  
  const int kMaxDevLogBufferSize = 2000;// 60 seconds / 30ms = 2000 samples
  
  const f32 kSendWebVizDataPeriod_sec = 0.5f;
  f32 _nextSendWebVizDataTime_sec = 0.0f;
  size_t _touchCountForWebviz = 0;
  
} // end anonymous namespace
  
const TouchSensorComponent::FilterParameters TouchSensorComponent::_kFilterParamsForDVT =
{
    .boxFilterSize = 8,
    .dynamicThreshGap = 9,
    .dynamicThreshDetectFollowOffset = 7,
    .dynamicThreshUndetectFollowOffset = 7,
    .dynamicThreshMaximumDetectOffset = 50,
    .dynamicThreshMininumUndetectOffset = 13,
    .minConsecCountToUpdateThresholds = 8,
};
  
const TouchSensorComponent::FilterParameters TouchSensorComponent::_kFilterParamsForProd =
{
  .boxFilterSize = 55,
  .dynamicThreshGap = 1,
  .dynamicThreshDetectFollowOffset = 2,
  .dynamicThreshUndetectFollowOffset = 2,
  .dynamicThreshMaximumDetectOffset = 20,
  .dynamicThreshMininumUndetectOffset = 8,
  .minConsecCountToUpdateThresholds = 6,
};

TouchSensorComponent::TouchSensorComponent() 
: ISensorComponent(kLogDirectory)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::TouchSensor)
, _filterParams(_kFilterParamsForProd)
, _lastRawTouchValue(0)
, _noContactCounter(kMinCountNoTouchForAccumulation)
, _baselineTouch(kBaselineInitial)
, _boxFilterTouch(_filterParams.boxFilterSize)
, _detectOffsetFromBase(_filterParams.dynamicThreshMininumUndetectOffset+
                        _filterParams.dynamicThreshGap)
, _undetectOffsetFromBase(_filterParams.dynamicThreshMininumUndetectOffset)
, _isPressed(false)
, _numConsecCalibReadings(0)
, _countAboveDetectLevel(0)
, _countBelowUndetectLevel(0)
, _touchPressTime(std::numeric_limits<float>::max())
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
  const bool isDVThardware = (esn >= 0x00e00000 && esn <= 0x00e1ffff) &&
                              exceptionESNWithPVThardware.find(esn)==exceptionESNWithPVThardware.end();
  if(isDVThardware) {
    _filterParams = _kFilterParamsForDVT;
  } else {
    _filterParams = _kFilterParamsForProd;
  }
  _useFilterLogicForDVT = isDVThardware;
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
          const std::string& configStr = JsonTools::ParseString(data, "filterConfig", "TouchSensorComponent.WebVizJsonCallback");
          if(configStr.compare("DVT")==0) {
            _filterParams = _kFilterParamsForDVT;
            _useFilterLogicForDVT = true;
          } else {
            _filterParams = _kFilterParamsForProd;
            _useFilterLogicForDVT = false;
          }
          
          // recalibrate and re-init the boxfilter
          _numConsecCalibReadings = 0;
          _baselineTouch = kBaselineInitial;
          _boxFilterTouch = BoxFilter(_filterParams.boxFilterSize);
          
          // ack the mode change
          const auto* context = _robot->GetContext();
          if (context != nullptr) {
            auto* webService = context->GetWebService();
            if (webService != nullptr) {
              Json::Value toSend = Json::objectValue;
              toSend["mode"] = _useFilterLogicForDVT ? "DVT" : "Prod";
              webService->SendToWebViz(kWebVizTouchSensorModuleName, toSend);
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

void TouchSensorComponent::NotifyOfRobotStateInternal(const RobotState& msg)
{
  // send the information for webviz once
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if (now_sec > _nextSendWebVizDataTime_sec) {
    const auto* context = _robot->GetContext();
    if(context != nullptr) {
      auto* webService = context->GetWebService();
      if(webService != nullptr) {
        Json::Value toSend = Json::objectValue;
        auto* osstate = OSState::getInstance();
        toSend["esn"] = osstate->GetSerialNumberAsString();
        toSend["mode"] = _useFilterLogicForDVT ? "DVT" : "Prod";
        toSend["count"] = (int)_touchCountForWebviz;
        webService->SendToWebViz(kWebVizTouchSensorModuleName, toSend);
        _nextSendWebVizDataTime_sec = now_sec + kSendWebVizDataPeriod_sec;
      }
    }
  }
  
  if(!devEnableTouchSensor) {
    return;
  }
  
  if(FACTORY_TEST &&_dataToRecord != nullptr)
  {
    for(int i=0; i<STATE_MESSAGE_FREQUENCY; ++i) {
      _dataToRecord->data.push_back(msg.backpackTouchSensorRaw[i]);
    }
  }

  // sometimes spurious values that are absurdly high come through the sensor
  const bool valuesAreValid = std::all_of(msg.backpackTouchSensorRaw.begin(),
                                          msg.backpackTouchSensorRaw.end(),
                                          [](const uint16_t& value)->bool {
                                            return value <= kMaxTouchIntensityInvalid;
                                          });
  if(!valuesAreValid) {
    return;
  }
  
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  _lastRawTouchValue = msg.backpackTouchSensorRaw.back();
  
  // boxfilter on touch sensor values from the robot is treated differently
  // between DVT and production robots because of a change to the design of
  // the sensor that necessitates using more values from robot
  // thus we check the ESN of the robot to use the correct logic based on
  // the hardware for backwards compatibility
  int boxFiltVal = 0;
  if(_useFilterLogicForDVT) {
    boxFiltVal = _boxFilterTouch.ApplyFilter(msg.backpackTouchSensorRaw.back());
  } else {
    for(int i=0; i<STATE_MESSAGE_FREQUENCY; ++i) {
      boxFiltVal = _boxFilterTouch.ApplyFilter(msg.backpackTouchSensorRaw[i]);
    }
  }
  
  const bool isPickedUp = (msg.status & (uint32_t)RobotStatusFlag::IS_PICKED_UP) != 0;

  bool lastPressed = _isPressed; // after this tick, state may change
  
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
    const bool didChange = ProcessWithDynamicThreshold(_baselineTouch, boxFiltVal);
    if(didChange) {
      _robot->Broadcast(
        ExternalInterface::MessageEngineToGame(
          ExternalInterface::TouchButtonEvent(GetIsPressed())));
      if(GetIsPressed()) {
        _touchPressTime = now;
        _touchCountForWebviz++;
      }
    }
    // require seeing a minimum number of untouched cycles before
    // continuing accumulation of values for baseline detect
    DEV_ASSERT( IsCalibrated(), "TouchSensorComponent.ClassifyingBeforeCalibration");
    if(!GetIsPressed() && !isPickedUp) {
      _noContactCounter++;
      if(_noContactCounter >= kMinCountNoTouchForAccumulation) {
        _baselineTouch = kBaselineFilterCoeff*boxFiltVal + (1.0f-kBaselineFilterCoeff)*_baselineTouch;
        _noContactCounter = kMinCountNoTouchForAccumulation;
      }
    } else {
      // if we are picked up OR we are being touched
      _noContactCounter = 0;
    }
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
  {
    DevLogTouchRow entry
    {
      _lastRawTouchValue,
      boxFiltVal,
      (int)_baselineTouch,
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
  // if input touch is within deadband for detect/undetect
  // then there will be no change in these values
  // Additionally _isPressed state won't change
  bool lastPressed = _isPressed;
  
  const int detectLevel = baseline + _detectOffsetFromBase;
  const int undetectLevel = baseline + _undetectOffsetFromBase;
  
  DEV_ASSERT( detectLevel >= undetectLevel, "DetectLevelLowerThanUndetectLevel");
  
  if (input > detectLevel) {
    _isPressed = true;
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
    _isPressed = false;
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
