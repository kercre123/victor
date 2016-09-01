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

#include "anki/cozmo/basestation/behaviors/behaviorPlayAnimSequence.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static const char* kAnimTriggerKey = "animTriggers";
static const char* kLoopsKey = "num_loops";

BehaviorPlayAnimSequence::BehaviorPlayAnimSequence(Robot& robot, const Json::Value& config, bool triggerRequired)
: IBehavior(robot, config)
, _numLoops(0)
, _sequenceLoopsDone(0)
{
  SetDefaultName("PlayAnimSequence");

  // load anim triggers
  for( const auto& trigger : config[kAnimTriggerKey] )
  {
    // make sure each trigger is valid
    const AnimationTrigger animTrigger = AnimationTriggerFromString( trigger.asString().c_str() );
    if ( animTrigger != AnimationTrigger::Count ) {
      _animTriggers.emplace_back( animTrigger );
    } else {
      ASSERT_NAMED_EVENT(animTrigger != AnimationTrigger::Count, "BehaviorPlayAnimSequence.InvalidTriggerString",
        "'%s'", trigger.asString().c_str() );
    }
  }
  // make sure we loaded at least one trigger
  ASSERT_NAMED_EVENT(!triggerRequired || !_animTriggers.empty(), "BehaviorPlayAnimSequence.NoTriggers", "Behavior '%s'", GetName().c_str());
 
  // load loop count
  _numLoops = config.get(kLoopsKey, 1).asInt();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayAnimSequence::~BehaviorPlayAnimSequence()
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayAnimSequence::IsRunnableInternal(const Robot& robot) const
{
  const bool hasAnims = !_animTriggers.empty();
  return hasAnims;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPlayAnimSequence::InitInternal(Robot& robot)
{
  ASSERT_NAMED(!_animTriggers.empty(), "BehaviorPlayAnimSequence.InitInternal.NoTriggers");
  
  // start first anim
  if ( _animTriggers.size() == 1 )
  {
    // simple anim, action loops
    const AnimationTrigger animTrigger = _animTriggers[0];
    StartActing(new TriggerLiftSafeAnimationAction(robot, animTrigger, _numLoops));
  }
  else
  {
    // multiple anims, play sequence loop
    _sequenceLoopsDone = 0;
    StartSequenceLoop(robot);
  }

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimSequence::StartSequenceLoop(Robot& robot)
{
  // if not done, start another sequence
  if ( _sequenceLoopsDone < _numLoops )
  {
    // create sequence with all triggers
    CompoundActionSequential* sequenceAction = new CompoundActionSequential(robot);
    for( AnimationTrigger trigger : _animTriggers ) {
      IAction* playAnim = new TriggerLiftSafeAnimationAction(robot, trigger, 1);
      sequenceAction->AddAction(playAnim);
    }
    // count already that the loop is done for the next time
    ++_sequenceLoopsDone;
    // start it and come back here next time to check for more loops
    StartActing(sequenceAction, &BehaviorPlayAnimSequence::StartSequenceLoop);
  }
}

} // namespace Cozmo
} // namespace Anki
