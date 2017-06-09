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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqNone.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayArbitraryAnim::BehaviorPlayArbitraryAnim(Robot& robot, const Json::Value& config)
: BehaviorPlayAnimSequence(robot, config, false)
, _animationAlreadySet(false)
{
  _numLoops = -1;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayArbitraryAnim::~BehaviorPlayArbitraryAnim()
{  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayArbitraryAnim::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  BehaviorPreReqNone noPreReqs;
  const bool retVal = _numLoops >= 0 && BaseClass::IsRunnableInternal(noPreReqs);
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
Result BehaviorPlayArbitraryAnim::InitInternal(Robot& robot)
{
  _animationAlreadySet = false;
  BaseClass::StartPlayingAnimations(robot);
  return Result::RESULT_OK;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPlayArbitraryAnim::ResumeInternal(Robot& robot)
{
  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayArbitraryAnim::StopInternal(Robot& robot)
{
  _animationAlreadySet = false;
  _animTriggers.clear();
}


} // namespace Cozmo
} // namespace Anki
