/**
 * File: iSensorComponent.cpp
 *
 * Author: Matt Michini
 * Created: 10/26/2017
 *
 * Description: Interface for components that manage sensors. Handles logging
 * and provides an interface for updating with a RobotState message.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/sensors/iSensorComponent.h"

#include "engine/robot.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/logging/rollingFileLogger.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirectoryBase = "sensorData";
}

  
ISensorComponent::ISensorComponent(const std::string& logDirName)
: _rawDataLogger(nullptr)
, _logDirectory(kLogDirectoryBase + "/" + logDirName)
{
}


ISensorComponent::~ISensorComponent() = default;

void ISensorComponent::NotifyOfRobotState(const RobotState& msg)
{
  // Update the derived class
  NotifyOfRobotStateInternal(msg);
  
  // Log stuff if necessary
  Log();
}


void ISensorComponent::StartLogging(const uint32_t duration_ms)
{
  if (!_loggingRawData) {
    _loggingRawData = true;
    if (duration_ms != 0) {
      const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _logRawDataUntil_s = now + static_cast<float>(duration_ms)/1000.f;
    } else {
      _logRawDataUntil_s = 0.f;
    }
    PRINT_NAMED_INFO("ISensorComponent.StartLogging.Start",
                     "Starting raw data logging, duration %d ms%s. Log will appear in '%s'",
                     duration_ms,
                     _logRawDataUntil_s == 0.f ? " (indefinitely)" : "",
                     _robot->GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, _logDirectory).c_str());
  } else {
    PRINT_NAMED_WARNING("ISensorComponent.StartLogging.AlreadyLogging", "Already logging raw data!");
  }
}
  
  
void ISensorComponent::StopLogging()
{
  if (_loggingRawData) {
    const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _logRawDataUntil_s = now;
  } else {
    PRINT_NAMED_WARNING("ISensorComponent.StopLogging.NotLogging", "Not logging raw data!");
  }
}
  
void ISensorComponent::Log()
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
    _rawDataLogger = std::make_unique<Util::RollingFileLogger>(nullptr, _robot->GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, _logDirectory));
    auto header = GetLogHeader();
    DEV_ASSERT((header.find('\n') == std::string::npos) && (header.find('\r') == std::string::npos), "ISensorComponent.Log.LinebreakInHeaderNotAllowed");
    header += "\n";
    _rawDataLogger->Write(header);
  }
  
  auto row = GetLogRow();
  DEV_ASSERT((row.find('\n') == std::string::npos) && (row.find('\r') == std::string::npos), "ISensorComponent.Log.LinebreakInRowNotAllowed");
  row += "\n";
  _rawDataLogger->Write(row);
}

  
} // Cozmo namespace
} // Anki namespace
