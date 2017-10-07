/**
 * File: behaviorPlayAnimSequence
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimSequence.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static const char* kAnimTriggerKey = "animTriggers";
static const char* kLoopsKey = "num_loops";

BehaviorPlayAnimSequence::BehaviorPlayAnimSequence(const Json::Value& config, bool triggerRequired)
: ICozmoBehavior(config)
, _numLoops(0)
, _sequenceLoopsDone(0)
{
  // load anim triggers
  for (const auto& trigger : config[kAnimTriggerKey])
  {
    // make sure each trigger is valid
    const AnimationTrigger animTrigger = AnimationTriggerFromString(trigger.asString().c_str());
    DEV_ASSERT_MSG(animTrigger != AnimationTrigger::Count, "BehaviorPlayAnimSequence.InvalidTriggerString",
                   "'%s'", trigger.asString().c_str());
    if (animTrigger != AnimationTrigger::Count) {
      _animTriggers.emplace_back( animTrigger );
    }
  }
  // make sure we loaded at least one trigger
  DEV_ASSERT_MSG(!triggerRequired || !_animTriggers.empty(), "BehaviorPlayAnimSequence.NoTriggers",
                 "Behavior '%s'", GetIDStr().c_str());
 
  // load loop count
  _numLoops = config.get(kLoopsKey, 1).asInt();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayAnimSequence::~BehaviorPlayAnimSequence()
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayAnimSequence::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool hasAnims = !_animTriggers.empty();
  return hasAnims && IsRunnableAnimSeqInternal(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPlayAnimSequence::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  StartPlayingAnimations(behaviorExternalInterface);

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimSequence::StartPlayingAnimations(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEV_ASSERT(!_animTriggers.empty(), "BehaviorPlayAnimSequence.InitInternal.NoTriggers");
  
  // start first anim
  if (_animTriggers.size() == 1)
  {
    // simple anim, action loops
    const AnimationTrigger animTrigger = _animTriggers[0];
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(robot, animTrigger, _numLoops),
                &BehaviorPlayAnimSequence::CallToListeners);
  }
  else
  {
    // multiple anims, play sequence loop
    _sequenceLoopsDone = 0;
    StartSequenceLoop(behaviorExternalInterface);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimSequence::StartSequenceLoop(BehaviorExternalInterface& behaviorExternalInterface)
{
  // if not done, start another sequence
  if (_sequenceLoopsDone < _numLoops)
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    // create sequence with all triggers
    CompoundActionSequential* sequenceAction = new CompoundActionSequential(robot);
    for (AnimationTrigger trigger : _animTriggers) {
      IAction* playAnim = new TriggerLiftSafeAnimationAction(robot, trigger, 1);
      sequenceAction->AddAction(playAnim);
    }
    // count already that the loop is done for the next time
    ++_sequenceLoopsDone;
    // start it and come back here next time to check for more loops
    DelegateIfInControl(sequenceAction, [this](BehaviorExternalInterface& behaviorExternalInterface) {
      CallToListeners(behaviorExternalInterface);
      StartSequenceLoop(behaviorExternalInterface);
    });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimSequence::AddListener(ISubtaskListener* listener)
{
  _listeners.insert(listener);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimSequence::CallToListeners(BehaviorExternalInterface& behaviorExternalInterface)
{
  for(auto& listener : _listeners)
  {
    listener->AnimationComplete(behaviorExternalInterface);
  }
}

} // namespace Cozmo
} // namespace Anki
