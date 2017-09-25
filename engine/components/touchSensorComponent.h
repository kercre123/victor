/**
 * File: touchSensorComponent.h
 *
 * Author: Arjun Menon
 * Created: 9/7/2017
 *
 * Description: Component for managing the capacitative touch sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_Components_TouchSensorComponent_H__
#define __Engine_Components_TouchSensorComponent_H__

#include "anki/common/types.h"

#include "clad/types/proxMessages.h"

#include "util/helpers/noncopyable.h"
#include <memory>

namespace Anki {
namespace Util {
  class RollingFileLogger;
}
namespace Cozmo {

class Robot;
struct RobotState;

class TouchSensorComponent : private Util::noncopyable
{
public:
  // constructor/destructor
  TouchSensorComponent(Robot& robot);
  ~TouchSensorComponent();

  void Update(const RobotState& msg);
  
  // Start logging raw data from the sensor for the specified duration.
  // Specifying 0 for duration will continue logging indefinitely
  void StartLogging(const uint32_t duration_ms = 0);
  void StopLogging();
  
private:

  void Log();
  
  Robot& _robot;

  // The timestamp of the RobotState message with the
  // latest capacitive touch sensor data
  uint32_t _lastMsgTimestamp = 0;
  uint16_t _latestData;

  
  // Logging stuff:
  std::unique_ptr<Util::RollingFileLogger> _rawDataLogger;
  bool _loggingRawData = false;
  float _logRawDataUntil_s = 0.f;
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_TouchSensorComponent_H__
