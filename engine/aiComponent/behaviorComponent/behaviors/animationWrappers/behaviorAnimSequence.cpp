/**
 * File: behaviorAnimSequence
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimSequence.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/events/animationTriggerHelpers.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
namespace{
static const char* kAnimTriggerKey = "animTriggers";
static const char* kAnimNamesKey   = "animNames";
static const char* kLoopsKey       = "num_loops";
static const char* kSupportCharger = "playOnChargerWithoutBody";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequence::InstanceConfig::InstanceConfig() {
  activatableOnCharger = false;
  numLoops = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequence::DynamicVariables::DynamicVariables() {
  sequenceLoopsDone = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequence::BehaviorAnimSequence(const Json::Value& config, bool triggerRequired)
: ICozmoBehavior(config)
{
  // load anim triggers, if they exist
  for (const auto& trigger : config[kAnimTriggerKey])
  {
    // make sure each trigger is valid
    const AnimationTrigger animTrigger = AnimationTriggerFromString(trigger.asString().c_str());
    DEV_ASSERT_MSG(animTrigger != AnimationTrigger::Count, "BehaviorAnimSequence.InvalidTriggerString",
                  "'%s'", trigger.asString().c_str());
    if (animTrigger != AnimationTrigger::Count) {
      _iConfig.animTriggers.emplace_back( animTrigger );
    }
  }

  // load animations by name, if they exist
  for (const auto& animName : config[kAnimNamesKey])
  {
    _iConfig.animationNames.emplace_back( animName.asString() );
  }

  if(ANKI_DEV_CHEATS){
    const bool onlyTriggersSet = !_iConfig.animTriggers.empty() && _iConfig.animationNames.empty();
    const bool onlyNamesSet    =  _iConfig.animTriggers.empty() && !_iConfig.animationNames.empty();
    // make sure we loaded at least one trigger or animation by name
    DEV_ASSERT_MSG(!triggerRequired || onlyTriggersSet || onlyNamesSet,
                   "BehaviorAnimSequence.NoTriggers", "Behavior '%s'", GetDebugLabel().c_str());
  }

  // load loop count
  _iConfig.numLoops = config.get(kLoopsKey, 1).asInt();

  _iConfig.activatableOnCharger = config.get(kSupportCharger, false).asBool();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequence::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kAnimTriggerKey,
    kAnimNamesKey,
    kLoopsKey,
    kSupportCharger,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequence::~BehaviorAnimSequence()
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAnimSequence::WantsToBeActivatedBehavior() const
{
  const bool hasAnims = !_iConfig.animTriggers.empty() || !_iConfig.animationNames.empty();
  return hasAnims && WantsToBeActivatedAnimSeqInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequence::OnBehaviorActivated()
{
  StartPlayingAnimations();

  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequence::StartPlayingAnimations()
{
  DEV_ASSERT(!_iConfig.animTriggers.empty() || !_iConfig.animationNames.empty(), "BehaviorAnimSequence.InitInternal.NoTriggers");
  if(IsSequenceLoop()){
    _dVars.sequenceLoopsDone = 0;    
    StartSequenceLoop();
  }else{
    IActionRunner* action = GetAnimationAction();    
    DelegateIfInControl(action, [this]() {
      CallToListeners();
    });
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequence::StartSequenceLoop()
{
    // if not done, start another sequence
    if (_dVars.sequenceLoopsDone < _iConfig.numLoops)
    {
      IActionRunner* action = GetAnimationAction();
      // count already that the loop is done for the next time
      ++_dVars.sequenceLoopsDone;
      // start it and come back here next time to check for more loops
      DelegateIfInControl(action, [this]() {
        CallToListeners();
        StartSequenceLoop();
      });
    }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorAnimSequence::GetAnimationAction()
{
  u32 numLoops = _iConfig.numLoops; 
  if(IsSequenceLoop()){
    numLoops = 1; // just one loop per animation, so we can loop the entire sequence together
  }

  // create sequence with all triggers
  CompoundActionSequential* sequenceAction = new CompoundActionSequential();
  for (AnimationTrigger trigger : _iConfig.animTriggers) {
    const bool interruptRunning = true;
    const u8 tracksToLock = GetTracksToLock();

    IAction* playAnim = new TriggerLiftSafeAnimationAction(trigger,
                                                           numLoops,
                                                           interruptRunning,
                                                           tracksToLock);
    sequenceAction->AddAction(playAnim);
  }

  for (const auto& name : _iConfig.animationNames) {
    const bool interruptRunning = true;
    const u8 tracksToLock = GetTracksToLock();

    IAction* playAnim = new PlayAnimationAction(name,
                                                numLoops,
                                                interruptRunning,
                                                tracksToLock);
    sequenceAction->AddAction(playAnim);
  }
  return sequenceAction;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAnimSequence::IsSequenceLoop()
{
  return (_iConfig.animTriggers.size() > 1) || 
         (_iConfig.animationNames.size() > 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequence::AddListener(ISubtaskListener* listener)
{
  _dVars.listeners.insert(listener);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequence::CallToListeners()
{
  for(auto& listener : _dVars.listeners)
  {
    listener->AnimationComplete();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u8 BehaviorAnimSequence::GetTracksToLock() const
{
  if( _iConfig.activatableOnCharger ) {
    const auto& robotInfo = GetBEI().GetRobotInfo();
    if( robotInfo.IsOnChargerPlatform() ) {
      // we are supporting the charger and are on it, so lock out the body
      return (u8)AnimTrackFlag::BODY_TRACK;
    }
  }

  // otherwise nothing to lock
  return (u8)AnimTrackFlag::NO_TRACKS;   
}


} // namespace Cozmo
} // namespace Anki
