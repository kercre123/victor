/**
 * File: progressionUnlockEntry.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-04
 *
 * Description: Entry to store data associated with a given unlock
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_ProgressionUnlockEntry_H__
#define __Anki_Cozmo_Basestation_Components_ProgressionUnlockEntry_H__

#include "clad/types/behaviorGroup.h"
#include "json/json-forwards.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class ProgressionUnlockEntry
{
public:

  explicit ProgressionUnlockEntry(const Json::Value& config);

  void SetUnlock(bool unlocked);
  bool IsUnlocked() const;
  
  bool HasFreeplayBehaviorGroup() const;
  BehaviorGroup GetFreeplayBehaviorGroup() const;

private:

  bool _enabled;
  BehaviorGroup _freeplayBehaviorGroup;
  
};

}
}

#endif
