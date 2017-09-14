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
} // end anonymous namespace

  
TouchSensorComponent::TouchSensorComponent(Robot& robot)
  : _robot(robot)
  , _rawDataLogger(nullptr)
{
}
  
  
TouchSensorComponent::~TouchSensorComponent() = default;
  
  
void TouchSensorComponent::Update(const RobotState& msg)
{
  _lastMsgTimestamp = msg.timestamp;

  _latestData = msg.backpackTouchSensorRaw;
  
  Log();
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
  str += std::to_string(_lastMsgTimestamp)  + ", ";
  str += std::to_string(_latestData)        + "\n";
  _rawDataLogger->Write(str);
}

  
} // Cozmo namespace
} // Anki namespace
