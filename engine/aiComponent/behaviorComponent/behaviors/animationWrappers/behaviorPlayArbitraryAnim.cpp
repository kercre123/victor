/**
 * File: BehaviorPlayArbitraryAnim.cpp
 *
 * Author: Kevin M. Karol
 * Created: 08/17/16
 *
 * Description: Behavior that can be used to play an arbitrary animation computationally
 * Should not be used as type for a behavior created from JSONs - should be created on demand
 * with the factory and owned by the chooser creating them so that other parts of the system
 * don't re-set the animation trigger in a race condition
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "engine/actions/animActions.h"
#include "engine/events/animationTriggerHelpers.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayArbitraryAnim::BehaviorPlayArbitraryAnim(const Json::Value& config)
: BehaviorPlayAnimSequence(config, false)
, _animationAlreadySet(false)
{
  _numLoops = -1;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayArbitraryAnim::~BehaviorPlayArbitraryAnim()
{  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayArbitraryAnim::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool retVal = _numLoops >= 0 && BaseClass::WantsToBeActivatedBehavior(behaviorExternalInterface);
  return retVal;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayArbitraryAnim::SetAnimationTrigger(AnimationTrigger trigger, int numLoops)
{
  DEV_ASSERT_MSG(!_animationAlreadySet, "BehaviorPlayArbitraryAnim.SetAnimationTrigger",
                 "Animation set twice before being played");
  
  // clear current triggers and add new one
  _animTriggers.clear();
  _animTriggers.emplace_back(trigger);
  
  _numLoops = numLoops;
  _animationAlreadySet = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayArbitraryAnim::SetAnimationTriggers(std::vector<AnimationTrigger>& triggers, int sequenceLoopCount)
{
  DEV_ASSERT_MSG(!_animationAlreadySet, "BehaviorPlayArbitraryAnim.SetAnimationTriggers",
                 "Animation set twice before being played");

  // clear current triggers and add new ones
  _animTriggers.clear();
  for (AnimationTrigger trigger : triggers) {
    DEV_ASSERT(trigger != AnimationTrigger::Count, "BehaviorPlayArbitraryAnim.InvalidTriggerPassedIn");
    _animTriggers.emplace_back(trigger);
  }
  
  _numLoops = sequenceLoopCount;
  _animationAlreadySet = true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPlayArbitraryAnim::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _animationAlreadySet = false;
  BaseClass::StartPlayingAnimations(behaviorExternalInterface);
  return Result::RESULT_OK;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPlayArbitraryAnim::ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayArbitraryAnim::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _animationAlreadySet = false;
  _animTriggers.clear();
}


} // namespace Cozmo
} // namespace Anki
