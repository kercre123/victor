/**
* File: behaviorEventAnimResponseDirector.cpp
*
* Author: Kevin M. Karol
* Created: 2/24/17
*
* Description: Tracks the appropriate response that should be played if Cozmo
* encounters a behavior event.
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/behaviorEventAnimResponseDirector.h"

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEventAnimResponseDirector::BehaviorEventAnimResponseDirector()
{
  _animationTriggerMap[UserFacingActionResult::InteractWithBlockDockingIssue] = AnimationTrigger::RollBlockRetry;
  _animationTriggerMap[UserFacingActionResult::DriveToBlockIssue] = AnimationTrigger::RollBlockRealign;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorEventAnimResponseDirector::GetAnimationToPlayForActionResult(const UserFacingActionResult result) const
{
  const auto entry = _animationTriggerMap.find(result);
  if(entry != _animationTriggerMap.end()){
    return entry->second;
  }
  
  return AnimationTrigger::Count;
}


} // namespace Cozmo
} // namespace Anki
