/**
 * File: freeplayDataTracker.h
 *
 * Author: Brad Neuman
 * Created: 2017-08-03
 *
 * Description: Component to track things that happen in freeplay and send DAS events
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_AiComponent_FreeplayDataTracker_H__
#define __Cozmo_Basestation_AiComponent_FreeplayDataTracker_H__

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"

#include "util/helpers/noncopyable.h"

#include <set>

namespace Anki {
namespace Vector {

enum class FreeplayPauseFlag
{
  GameControl, // engine is not in control of the robot (e.g. a game is running from Unity)
  Spark,
  OffTreads,
  OnCharger
};

class FreeplayDataTracker : public IDependencyManagedComponent<AIComponentID>, 
                            private Util::noncopyable
{
public:

  FreeplayDataTracker();

  // Force a data send (if there is data to send). Useful, e.g. during a disconnect
  void ForceUpdate();

  // set (or clear) the given freeplay pause flag. If no flags are set, freeplay is not paused.
  void SetFreeplayPauseFlag(bool value, FreeplayPauseFlag flag);

  // IDependencyManagedComponent<AIComponentID> functions
  virtual void InitDependent(Vector::Robot* robot, 
                             const AICompMap& dependentComps) override;

  // IDependencyManagedComponent<AIComponentID> functions
  virtual void GetUpdateDependencies(AICompIDSet& dependencies) const override {
    dependencies.insert(AIComponentID::BehaviorComponent);
  };

  virtual void UpdateDependent(const AICompMap& dependentComps) override;
  // end IDependencyManagedComponent<AIComponentID> functions


private:

  // This is a set of flags, each of which is a reason we might be paused. Multiple can be active at a
  // time. If any flags are set, then we are paused. Otherwise, freeplay is active
  std::set< FreeplayPauseFlag > _pausedFlags;

  // engine timestamp we are counting from for the current active freeplay session
  BaseStationTime_t _countFreeplayFromTime_ns = 0;

  // next time (in seconds) that we should send data
  float _timeToSendData_s = 0.0f;

  // how much data we have accumulated to send
  BaseStationTime_t _timeData_ns = 0;

  void SetFreeplayPauseFlag(FreeplayPauseFlag flag);
  void ClearFreeplayPauseFlag(FreeplayPauseFlag flag);

  void SendData();

  static const char* FreeplayPauseFlagToString(FreeplayPauseFlag flag);

  bool IsActive() const;

  // return a string representing the current pause state of the system
  std::string GetDebugStateStr() const;

  
};

}
}

#endif
