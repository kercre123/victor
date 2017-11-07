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
static const char* kAnimNamesKey   = "animNames";
static const char* kLoopsKey       = "num_loops";
static const char* kSupportCharger = "playOnChargerWithoutBody";

BehaviorPlayAnimSequence::BehaviorPlayAnimSequence(const Json::Value& config, bool triggerRequired)
: ICozmoBehavior(config)
, _numLoops(0)
, _sequenceLoopsDone(0)
{
  

  // load anim triggers, if they exist
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

  // load animations by name, if they exist
  for (const auto& animName : config[kAnimNamesKey])
  {
    _animationNames.emplace_back( animName.asString() );
  }

  if(ANKI_DEV_CHEATS){
    const bool onlyTriggersSet = !_animTriggers.empty() && _animationNames.empty();
    const bool onlyNamesSet    =  _animTriggers.empty() && !_animationNames.empty();
    // make sure we loaded at least one trigger or animation by name
    DEV_ASSERT_MSG(!triggerRequired || onlyTriggersSet || onlyNamesSet,
                   "BehaviorPlayAnimSequence.NoTriggers", "Behavior '%s'", GetIDStr().c_str());
  }

  // load loop count
  _numLoops = config.get(kLoopsKey, 1).asInt();

  _supportCharger = config.get(kSupportCharger, false).asBool();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayAnimSequence::~BehaviorPlayAnimSequence()
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayAnimSequence::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool hasAnims = !_animTriggers.empty() || !_animationNames.empty();
  return hasAnims && WantsToBeActivatedAnimSeqInternal(behaviorExternalInterface);
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
  DEV_ASSERT(!_animTriggers.empty() || !_animationNames.empty(), "BehaviorPlayAnimSequence.InitInternal.NoTriggers");
  if(IsSequenceLoop()){
    _sequenceLoopsDone = 0;    
    StartSequenceLoop(behaviorExternalInterface);
  }else{
    IActionRunner* action = GetAnimationAction(behaviorExternalInterface);    
    DelegateIfInControl(action, [this](BehaviorExternalInterface& behaviorExternalInterface) {
      CallToListeners(behaviorExternalInterface);
    });
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimSequence::StartSequenceLoop(BehaviorExternalInterface& behaviorExternalInterface)
{
    // if not done, start another sequence
    if (_sequenceLoopsDone < _numLoops)
    {
      IActionRunner* action = GetAnimationAction(behaviorExternalInterface);
      // count already that the loop is done for the next time
      ++_sequenceLoopsDone;
      // start it and come back here next time to check for more loops
      DelegateIfInControl(action, [this](BehaviorExternalInterface& behaviorExternalInterface) {
        CallToListeners(behaviorExternalInterface);
        StartSequenceLoop(behaviorExternalInterface);
      });
    }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorPlayAnimSequence::GetAnimationAction(BehaviorExternalInterface& behaviorExternalInterface)
{
  u32 numLoops = _numLoops; 
  if(IsSequenceLoop()){
    numLoops = 1; // just one loop per animation, so we can loop the entire sequence together
  }
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // create sequence with all triggers
  CompoundActionSequential* sequenceAction = new CompoundActionSequential(robot);
  for (AnimationTrigger trigger : _animTriggers) {
    const bool interruptRunning = true;
    const u8 tracksToLock = GetTracksToLock(behaviorExternalInterface);

    IAction* playAnim = new TriggerLiftSafeAnimationAction(robot,
                                                            trigger,
                                                            numLoops,
                                                            interruptRunning,
                                                            tracksToLock);
    sequenceAction->AddAction(playAnim);
  }

  for (const auto& name : _animationNames) {
    const bool interruptRunning = true;
    const u8 tracksToLock = GetTracksToLock(behaviorExternalInterface);

    IAction* playAnim = new PlayAnimationAction(robot,
                                                name,
                                                numLoops,
                                                interruptRunning,
                                                tracksToLock);
    sequenceAction->AddAction(playAnim);
  }
  return sequenceAction;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayAnimSequence::IsSequenceLoop()
{
  return (_animTriggers.size() > 1) || 
         (_animationNames.size() > 1);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u8 BehaviorPlayAnimSequence::GetTracksToLock(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if( _supportCharger ) {
    const Robot& robot = behaviorExternalInterface.GetRobot();
    if( robot.IsOnChargerPlatform() ) {
      // we are supporting the charger and are on it, so lock out the body
      return (u8)AnimTrackFlag::BODY_TRACK;
    }
  }

  // otherwise nothing to lock
  return (u8)AnimTrackFlag::NO_TRACKS;   
}


} // namespace Cozmo
} // namespace Anki
