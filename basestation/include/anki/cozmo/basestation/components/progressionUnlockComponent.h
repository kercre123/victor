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

#include "anki/cozmo/basestation/components/progressionUnlockEntry.h"
#include "clad/types/unlockTypes.h"
#include "json/json-forwards.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal.hpp"
#include <map>
#include <vector>

namespace Anki {
namespace Cozmo {

class Robot;

class ProgressionUnlockComponent : private Util::noncopyable
{
public:

  explicit ProgressionUnlockComponent(Robot& robot);

  void Init(const Json::Value& config);

  bool IsUnlocked(UnlockId unlock) const;

  // returns false if there was an error, true otherwise. Note that this function returning true does not
  // guarantee that the robot is storing the value.
  bool SetUnlock(UnlockId unlock, bool unlocked);

  // send a message representing the current status of all unlocks
  void SendUnlockStatus() const;

  // Calls the provided function with the behavior group corresponding to each unlocked unlock NOTE: this
  // assumes that no two unlocks use the same behavior group. If they do, bad things may happen
  using UnlockBehaviorCallback = std::function<void(BehaviorGroup group, bool enabled)>;
  void IterateUnlockedFreeplayBehaviors(UnlockBehaviorCallback callback);

  void Update();

  template<typename T>
  void HandleMessage(const T& msg);

private:

  Robot& _robot;

  // eventually this will be stored on the robot
  std::map<UnlockId, ProgressionUnlockEntry> _unlocks;

  // for now, to simulate the robot "taking some time" to confirm values, delay a few ticks before
  // broadcasting confirmations
  std::multimap<float, UnlockId> _confirmationsToSend;

  std::vector<Signal::SmartHandle> _signalHandles;

};

}
}

#endif
