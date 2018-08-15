/**
 * File: iSensorComponent.h
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

#ifndef __Engine_Components_ISensorComponent_H__
#define __Engine_Components_ISensorComponent_H__

#include "coretech/common/shared/types.h"
#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include <memory>
#include <string>

namespace Anki {
namespace Util {
  class RollingFileLogger;
}
namespace Vector {

class Robot;
struct RobotState;

class ISensorComponent : private Util::noncopyable
{
public:
  // logDirName: Subdirectory where log files will live. Log files for the
  // derived sensor will appear in "cache/sensorData/<logDirName>"
  ISensorComponent(const std::string& logDirName);
  virtual ~ISensorComponent();

  void NotifyOfRobotState(const RobotState& msg);
  
  // Start logging raw data from the sensor for the specified duration.
  // Specifying 0 for duration will continue logging indefinitely
  void StartLogging(const uint32_t duration_ms = 0);
  void StopLogging();
  
protected:
  // derived classes pass the robot up to the base class
  void InitBase(Robot* robot) { _robot = robot;}

  // Derived class update function
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) = 0;
  
  // GetLogHeader() should return a comma-separated header line that describes the data being logged
  virtual std::string GetLogHeader() = 0;
  
  // GetLogRow() should return a comma-separated row of data for logging, with columns corresponding to the header
  virtual std::string GetLogRow() = 0;
  
  Robot* _robot = nullptr;
  
private:
  
  void Log();
  
  std::unique_ptr<Util::RollingFileLogger> _rawDataLogger;
  const std::string _logDirectory;
  bool _loggingRawData = false;
  float _logRawDataUntil_s = 0.f;
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ISensorComponent_H__
