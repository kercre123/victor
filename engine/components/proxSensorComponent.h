/**
 * File: proxSensorComponent.h
 *
 * Author: Matt Michini
 * Created: 8/30/2017
 *
 * Description: Component for managing forward distance sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_Components_ProxSensorComponent_H__
#define __Engine_Components_ProxSensorComponent_H__

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

class ProxSensorComponent : private Util::noncopyable
{
public:

  // constructor/destructor
  ProxSensorComponent(Robot& robot);
  ~ProxSensorComponent();

  void Update(const RobotState& msg);
  
  u16 GetLatestDistance_mm() const { return _latestData.distance_mm; }
  
  // Start logging raw data from the sensor for the specified duration.
  // Specifying 0 for duration will continue logging indefinitely
  void StartLogging(const uint32_t duration_ms = 0);
  void StopLogging();
  
private:

  void Log();
  
  Robot& _robot;

  ProxSensorData _latestData;
  
  // The timestamp of the RobotState message with the
  // latest distance sensor data
  uint32_t _lastMsgTimestamp = 0;
  
  // Logging stuff:
  std::unique_ptr<Util::RollingFileLogger> _rawDataLogger;
  bool _loggingRawData = false;
  float _logRawDataUntil_s = 0.f;
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ProxSensorComponent_H__
