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
#include "util/cladHelpers/cladFromJSONHelpers.h"

namespace Anki {
namespace Vector {
  
using namespace ExternalInterface;
  
namespace{
static const char* kAnimTriggerKey = "animTriggers";
static const char* kAnimNamesKey   = "animNames";
static const char* kLoopsKey       = "num_loops";
static const char* kTracksToLock   = "tracksToLock";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequence::InstanceConfig::InstanceConfig() {
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
    AnimationTrigger animTrigger;
    if( AnimationTriggerFromString( trigger.asString(), animTrigger ) ) 
    {
      _iConfig.animTriggers.emplace_back( animTrigger );
    }
    else 
    {
      PRINT_NAMED_ERROR("BehaviorAnimSequence.InvalidTriggerString",
                        "Behavior '%s' config specified an invalid animation trigger: '%s'",
                        GetDebugLabel().c_str(),
                        trigger.asCString() );
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
  
  _iConfig.tracksToLock = (u8)AnimTrackFlag::NO_TRACKS;
  if( !config[kTracksToLock].isNull() ) {
    for( const auto& trackStr : config[kTracksToLock] ) {
      if( ANKI_VERIFY(trackStr.isString(), "BehaviorAnimSequence.Ctor.TrackNotString", "Must be array of track strings" ) ) {
        AnimTrackFlag flag = AnimTrackFlag::NO_TRACKS;
        if( ANKI_VERIFY( AnimTrackFlagFromString(trackStr.asString(), flag),
                         "BehaviorAnimSequence.Ctor.InvalidTrack",
                         "Track '%s' is not a track", trackStr.asString().c_str() ) )
        {
          _iConfig.tracksToLock = _iConfig.tracksToLock | static_cast<u8>(flag);
        }
      }
    }
  }
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequence::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kAnimTriggerKey,
    kAnimNamesKey,
    kLoopsKey,
    kTracksToLock,
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
  if ( _iConfig.numLoops == 0 ||
       _dVars.sequenceLoopsDone < _iConfig.numLoops)
  {
    IActionRunner* action = GetAnimationAction();
    // count already that the loop is done for the next time
    ++_dVars.sequenceLoopsDone;
    // start it and come back here next time to check for more loops
    DelegateIfInControl(action, [this]() {
        CallToListeners();
        StartSequenceLoop();
      });
  } else {
    OnAnimationsComplete();
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

    // ********************************************************************************
    // ********************************************************************************
    // ************************************* BUG **************************************
    // ********************************* See VIC-7961 *********************************
    // ****** This action does not re-select from the group if numLoops is not 1 ******
    // ********************************************************************************
    // ********************************************************************************
    // ********************************************************************************

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
  // otherwise whatever was specified in config
  return _iConfig.tracksToLock;
}


} // namespace Vector
} // namespace Anki
