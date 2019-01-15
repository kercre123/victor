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
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/tofTypes.h"

namespace webots {
  class Supervisor;
}

namespace Anki {
namespace Cozmo {

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

  RangeDataRaw GetData(bool& hasDataUpdatedSinceLastCall);

  enum class CommandResult
    {
     Success = 0,
     Failure = -1,
     OpenLeftDevFailed,
     OpenRightDevFailed,
     SetupRightFailed,
     SetupLeftFailed,
     StartRangingLeftFailed,
     StartRangingRightFailed,
     StopRangingLeftFailed,
     StopRangingRightFailed,
     CalibrateRightFailed,
     CalibrateLeftFailed
    };
  // Callbacks are called on a thread
  using CommandCallback = std::function<void(CommandResult)>;
  int SetupSensors(const CommandCallback& callback);
  int StartRanging(const CommandCallback& callback);
  int StopRanging(const CommandCallback& callback);
  bool IsRanging();

  bool IsValidRoiStatus(uint8_t status);
  
#if FACTORY_TEST
  int PerformCalibration(uint32_t distanceToTarget_mm, float targetReflectance,
                         const CommandCallback& callback);
  bool IsCalibrating();
  void SetLogPath(const std::string& path);
#endif
  
private:
  
  ToFSensor();
  
  static ToFSensor* _instance;
};
  
}
}

#endif
