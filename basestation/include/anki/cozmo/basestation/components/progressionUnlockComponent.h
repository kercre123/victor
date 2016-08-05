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

  bool IsUnlocked(UnlockId unlock) const;

  // returns false if there was an error, true otherwise. Note that this function returning true does not
  // guarantee that the robot is storing the value.
  bool SetUnlock(UnlockId unlock, bool unlocked);

  // send a message representing the current status of all unlocks
  void SendUnlockStatus() const;

  void Update();

  template<typename T>
  void HandleMessage(const T& msg);

private:

  void WriteCurrentUnlocksToRobot(UnlockId id, bool unlocked);
  
  void ReadCurrentUnlocksFromRobot();
  
  bool IsUnlockIdValid(UnlockId id);

  Robot& _robot;

  std::set<UnlockId> _defaultUnlocks;

  // eventually this will be stored on the robot
  std::set<UnlockId> _currentUnlocks;

  std::vector<Signal::SmartHandle> _signalHandles;

};

}
}

#endif
