/**
 * File: tof.h
 *
 * Author: Al Chaussee
 * Created: 10/18/2018
 *
 * Description: Defines interface to a some number(2) of tof sensors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __platform_tof_h__
#define __platform_tof_h__

#include "coretech/common/shared/types.h"
#include "clad/types/tofTypes.h"

namespace webots {
  class Supervisor;
}

namespace Anki {
namespace Vector {

class ToFSensor
{
public:

  static ToFSensor* getInstance();
  static bool hasInstance() { return nullptr != _instance; }
  static void removeInstance();

  ~ToFSensor();

  Result Update();

#ifdef SIMULATOR

  static void SetSupervisor(webots::Supervisor* sup);

#endif

  // Get the latest ToF reading
  // hasDataUpdatedSinceLastCall indicates if the reading has changed since the last time this
  // function was called
  RangeDataRaw GetData(bool& hasDataUpdatedSinceLastCall);

  enum class CommandResult
  {
    Success = 0,
    Failure = -1,
    OpenDevFailed,
    SetupFailed,
    StartRangingFailed,
    StopRangingFailed,
    CalibrateFailed,
  };
  // All CommandCallbacks are called from a thread
  using CommandCallback = std::function<void(CommandResult)>;

  // Request the ToF device to be setup and configured for ranging
  int SetupSensors(const CommandCallback& callback);

  // Start ranging
  int StartRanging(const CommandCallback& callback);

  // Stop ranging
  int StopRanging(const CommandCallback& callback);

  // Whether or not the device is actively ranging
  bool IsRanging() const;

  // Whether or not the RoiStatus is considered valid
  bool IsValidRoiStatus(uint8_t status) const;
  
  // Run the calibration procedure at the given distance and target reflectance
  int PerformCalibration(uint32_t distanceToTarget_mm,
                         float targetReflectance,
                         const CommandCallback& callback);

  // Whether or not the device is currently calibrating
  bool IsCalibrating() const;

  // Set where to save calibration
  void SetLogPath(const std::string& path);
  
private:
  
  ToFSensor();
  
  static ToFSensor* _instance;
};
  
}
}

#endif
