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

#include "anki/cozmo/basestation/behaviors/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

BehaviorPlayArbitraryAnim::BehaviorPlayArbitraryAnim(Robot& robot, const Json::Value& config)
  : BehaviorPlayAnim(robot, config, false)
  , _animationAlreadySet(false)
{
  SetDefaultName("PlayArbitraryAnim");
  _animTrigger = AnimationTrigger::Count;
  _numLoops = -1;
}
    
BehaviorPlayArbitraryAnim::~BehaviorPlayArbitraryAnim()
{  
}
  
bool BehaviorPlayArbitraryAnim::IsRunnableInternal(const Robot& robot) const
{
  const bool retVal = _numLoops >= 0;
  return retVal;
}
  
void BehaviorPlayArbitraryAnim::SetAnimationTrigger(AnimationTrigger trigger, int numLoops)
{
  ASSERT_NAMED_EVENT(!_animationAlreadySet, "BehaviorPlayArbitraryAnim.SetAnimationTrigger",
                     "Animation set twice before being played");
  _animTrigger = trigger;
  _numLoops = numLoops;
  _animationAlreadySet = true;
}

  
Result BehaviorPlayArbitraryAnim::InitInternal(Robot& robot)
{
  _animationAlreadySet = false;
  return BaseClass::InitInternal(robot);
}



} // namespace Cozmo
} // namespace Anki
