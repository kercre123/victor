/**
 * File: progressionUnlockEntry.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-04
 *
 * Description: Entry to store data associated with a given unlock
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "anki/cozmo/basestation/components/progressionUnlockEntry.h"
#include "json/json.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

static const char* kFreeplayGroup = "freeplayBehaviorGroup";
static const char* kDefaultUnlocked = "unlocked";

ProgressionUnlockEntry::ProgressionUnlockEntry(const Json::Value& config)
  : _enabled( false )
  , _freeplayBehaviorGroup( BehaviorGroup::Count )
{
  if( ! config.isNull() ) {
    _enabled = config.get(kDefaultUnlocked, false).asBool();

    const Json::Value& freeplayGroup = config[kFreeplayGroup];
    
    if( !freeplayGroup.isNull() ) {
      _freeplayBehaviorGroup = BehaviorGroupFromString( freeplayGroup.asString() );
      if( _freeplayBehaviorGroup == BehaviorGroup::Count ) {
        PRINT_NAMED_WARNING("ProgressionUnlockEntry.InvalidConfig",
                            "behavior group '%s' invalid",
                            freeplayGroup.asString().c_str());
      }                            
    }
  }
}

void ProgressionUnlockEntry::SetUnlock(bool unlocked)
{
  _enabled = unlocked;
}

bool ProgressionUnlockEntry::IsUnlocked() const
{
  return _enabled;
}
  
bool ProgressionUnlockEntry::HasFreeplayBehaviorGroup() const
{
  return _freeplayBehaviorGroup != BehaviorGroup::Count;
}

BehaviorGroup ProgressionUnlockEntry::GetFreeplayBehaviorGroup() const
{
  return _freeplayBehaviorGroup;
}

}
}
