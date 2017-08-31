/**
 * File: proxSensorComponent.cpp
 *
 * Author: Matt Michini
 * Created: 8/30/2017
 *
 * Description: Component for managing forward distance sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/proxSensorComponent.h"

#include "engine/robot.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#include "util/logging/rollingFileLogger.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirectory = "sensorData/proxSensor";
} // end anonymous namespace

  
ProxSensorComponent::ProxSensorComponent(Robot& robot)
  : _robot(robot)
  , _rawDataLogger(nullptr)
{
}
  
  
ProxSensorComponent::~ProxSensorComponent() = default;
  
  
void ProxSensorComponent::Update(const RobotState& msg)
{
  _lastMsgTimestamp = msg.timestamp;

  _latestData = msg.proxData;
  
  Log();
}


void ProxSensorComponent::StartLogging(const uint32_t duration_ms)
{
  if (!_loggingRawData) {
    _loggingRawData = true;
    if (duration_ms != 0) {
      const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _logRawDataUntil_s = now + static_cast<float>(duration_ms)/1000.f;
    } else {
      _logRawDataUntil_s = 0.f;
    }
    PRINT_NAMED_INFO("ProxSensorComponent.StartLogging.Start",
                     "Starting raw cliff data logging, duration %d ms%s. Log will appear in '%s'",
                     duration_ms,
                     _logRawDataUntil_s == 0.f ? " (indefinitely)" : "",
                     _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory).c_str());
  } else {
    PRINT_NAMED_WARNING("ProxSensorComponent.StartLogging.AlreadyLogging", "Already logging raw data!");
  }
}
  
  
void ProxSensorComponent::StopLogging()
{
  if (_loggingRawData) {
    const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _logRawDataUntil_s = now;
  } else {
    PRINT_NAMED_WARNING("ProxSensorComponent.StopLogging.NotLogging", "Not logging raw data!");
  }
}

  
void ProxSensorComponent::Log()
{
  if (!_loggingRawData) {
    return;
  }
  
  // Check to see if we should stop logging:
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if (_logRawDataUntil_s != 0.f &&
      now >= _logRawDataUntil_s &&
      _rawDataLogger != nullptr) {
    _rawDataLogger->Flush();
    _rawDataLogger.reset();
    _loggingRawData = false;
    return;
  }
  
  // Create a logger if it doesn't exist already
  if (_rawDataLogger == nullptr) {
    _rawDataLogger = std::make_unique<Util::RollingFileLogger>(nullptr, _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, kLogDirectory));
    _rawDataLogger->Write("timestamp_ms, distance_mm, signalIntensity, ambientIntensity, spadCount \n");
  }
  
  const auto& d = _latestData;
  std::string str;
  str += std::to_string(_lastMsgTimestamp)  + ", ";
  str += std::to_string(d.distance_mm)      + ", ";
  str += std::to_string(d.signalIntensity)  + ", ";
  str += std::to_string(d.ambientIntensity) + ", ";
  str += std::to_string(d.spadCount)        + "\n";
  
  _rawDataLogger->Write(str);
}

  
} // Cozmo namespace
} // Anki namespace
