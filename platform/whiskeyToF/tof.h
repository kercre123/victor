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

  int SetupSensors();
  int StartRanging();
  int StopRanging();
  bool IsRanging();
  
#if FACTORY_TEST
  int PerformCalibration(uint32_t distanceToTarget_mm, float targetReflectance);
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
