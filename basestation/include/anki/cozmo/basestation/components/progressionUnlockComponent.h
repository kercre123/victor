/**
 * File: progressionUnlockComponent.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-04
 *
 * Description: A component to manage progression unlocks.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_ProgressionUnlockComponent_H__
#define __Anki_Cozmo_Basestation_Components_ProgressionUnlockComponent_H__

#include "clad/types/unlockTypes.h"
#include "json/json-forwards.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include <map>
#include <set>
#include <vector>

namespace Anki {
namespace Cozmo {

class Robot;

class ProgressionUnlockComponent : private Util::noncopyable
{
public:

  explicit ProgressionUnlockComponent(Robot& robot);

  void Init(const Json::Value &config);

  // By default, this checks if the given unlock is _actually_ unlocked. if forFreeplay=true, then first it
  // will check the freeplay overrides in json. If there is a corresponding entry there, this will return
  // true, even if the unlock is technically locked on the robot.
  bool IsUnlocked(UnlockId unlock, bool forFreeplay = false) const;

  // returns false if there was an error, true otherwise. Note that this function returning true does not
  // guarantee that the robot is storing the value.
  bool SetUnlock(UnlockId unlock, bool unlocked);

  // send a message representing the current status of all unlocks
  void SendUnlockStatus() const;

  void Update();

  template<typename T>
  void HandleMessage(const T& msg);
  
  static const std::set<UnlockId>& GetDefaultUnlocks();

private:

  void WriteCurrentUnlocksToRobot(UnlockId id, bool unlocked);
  
  void ReadCurrentUnlocksFromRobot();
  
  bool IsUnlockIdValid(UnlockId id);
  
  void NotifyGameDefaultUnlocksSet();

  Robot& _robot;

  // eventually this will be stored on the robot
  std::set<UnlockId> _currentUnlocks;

  // if the unlock is in here, always return true for IsUnlocked if asking from freeplay. This is a hack to
  // make Cozmo seem smarter out of the box, while some items are still locked.
  std::set<UnlockId> _freeplayOverrides;

  std::vector<Signal::SmartHandle> _signalHandles;

};

}
}

#endif
