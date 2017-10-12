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

#include "engine/components/touchSensorComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "clad/robotInterface/messageEngineToRobot.h"

// logging includes
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"
#include "util/logging/rollingFileLogger.h"

#include <memory>

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirectory = "sensorData/touchSensor";
  
  const uint32_t kNumTouchSamples = 30;  // store touch data for ~1s
                                    // sampled at 30ms update rate
                                    // 1/0.030 ~ 30 samples
  
  // const float kRobotTouchedMsgInterval_s = 0.75f;
  
  float kTouchIntensityThreshold = 625; // arbitrary
} // end anonymous namespace

  
TouchSensorComponent::TouchSensorComponent(Robot& robot)
  : _robot(robot)
  , _touchIntensitySamples(kNumTouchSamples)
  // , _lastTimeSentRobotTouchMsg_s(0.0f)
  , _rawDataLogger(nullptr)
{
}
  
  
TouchSensorComponent::~TouchSensorComponent() = default;
  
  
void TouchSensorComponent::Update(const RobotState& msg)
{
  if(msg.backpackTouchSensorRaw > 1000)
  {
    PRINT_NAMED_WARNING("TOUCH TOO LARGE", "");
    return;
  }

  // a circular buffer maintains the last touch intensity values in some constant time window
  _touchIntensitySamples.push_back(msg.backpackTouchSensorRaw);
  
  // not enough data to process touch data with
  if(_touchIntensitySamples.size() < kNumTouchSamples) {
    return;
  }
  
  // compute the mean from the latest subset of touch samples
  f32 meanTouchIntensity = 0.0;
  for(int i=0; i<kNumTouchSamples; ++i) {
    meanTouchIntensity += _touchIntensitySamples[i];
  }
  meanTouchIntensity /= kNumTouchSamples;

  static int c = 0;
  static f32 baseline = 0;
  if(c >= 0 && c < 100)
  {
    ++c;
    return;
  }
  else if(c != -1)
  {
    c = -1;
    baseline = meanTouchIntensity;
    kTouchIntensityThreshold = baseline + 30;
  }
  
  // PRINT_NAMED_WARNING("TOUCH INTENSITY", "%f", meanTouchIntensity);

  bool isBeingTouched     = meanTouchIntensity > kTouchIntensityThreshold;
  // const auto now          = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // const bool msgRateValid = (now-_lastTimeSentRobotTouchMsg_s)>kRobotTouchedMsgInterval_s;
  // if(isBeingTouched && msgRateValid) {
  //   _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotTouched()));
  //   _lastTimeSentRobotTouchMsg_s = now;
  // }

  if(isBeingTouched && !_wasTouched)
  {
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotTouched(true)));
  }
  else if(!isBeingTouched && _wasTouched)
  {
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotTouched(false)));
  }
  
  Log();
  
  _wasTouched = isBeingTouched;
}


void TouchSensorComponent::StartLogging(const uint32_t duration_ms)
{
  if (!_loggingRawData) {
    _loggingRawData = true;
    if (duration_ms != 0) {
      const auto now     = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _logRawDataUntil_s = now + static_cast<float>(duration_ms)/1000.f;
    } else {
      _logRawDataUntil_s = 0.f;
    }
    PRINT_NAMED_INFO("TouchSensorComponent.StartLogging.Start",
                     "Starting touch sensor data logging, duration %d ms%s. Log will appear in '%s'",
                     duration_ms,
                     _logRawDataUntil_s == 0.f ? " (indefinitely)" : "",
                     _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory).c_str());
  } else {
    PRINT_NAMED_WARNING("TouchSensorComponent.StartLogging.AlreadyLogging", "Already logging raw data!");
  }
}
  
  
void TouchSensorComponent::StopLogging()
{
  if (_loggingRawData) {
    const auto now     = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _logRawDataUntil_s = now;
  } else {
    PRINT_NAMED_WARNING("TouchSensorComponent.StopLogging.NotLogging", "Not logging raw data!");
  }
}

  
void TouchSensorComponent::Log()
{
  if (!_loggingRawData) {
    return;
  }
  
  // check if logging is complete, based on specified duration
  const auto now               = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool nonZeroLogEndTime = !NEAR_ZERO(_logRawDataUntil_s);
  const bool nowPastLogEndTime = now >= _logRawDataUntil_s;
  const bool loggerInit        = _rawDataLogger != nullptr;
  if( loggerInit && nonZeroLogEndTime && nowPastLogEndTime ) {
    _rawDataLogger->Flush();
    _rawDataLogger.reset();
    _loggingRawData = false;
    return;
  }
  
  // initialize logger
  if (_rawDataLogger == nullptr) {
    _rawDataLogger = std::make_unique<Util::RollingFileLogger>(nullptr, _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory));
    _rawDataLogger->Write("timestamp_ms, touchIntensity\n");
  }
  
  // append new data to logger
  std::string str;
  // XXX(agm): figure out what to log
  _rawDataLogger->Write(str);
}

  
} // Cozmo namespace
} // Anki namespace
