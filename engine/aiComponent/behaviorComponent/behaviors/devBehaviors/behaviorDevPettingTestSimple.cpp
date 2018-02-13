/**
 * File: BehaviorDevPettingTestSimple.cpp
 *
 * Author: Arjun Menon
 * Date:   09/12/2017
 *
 * Description: simple test behavior to respond to touch
 *              and petting input. Does nothing until a
 *              touch event comes in for it to react to
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevPettingTestSimple.h"

#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/engine/jsonTools.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"

// audio
#include "engine/audio/engineRobotAudioClient.h"
#include "clad/audio/audioEventTypes.h"

#include <vector>
#include <memory>
#include <limits>

namespace Anki {
namespace Cozmo {

namespace {
  const int kAudioVolumeUpTickPeriodBliss = 1;
  const int kAudioVolumeUpIncrementBliss = 2;
  
  const int kAudioVolumeUpIncrementNonbliss = 1;
  
  // animation control constants
  const int kPlayAnimForever = 0; // value for number of loops argument for infinitely playing anim
  const int kPlayAnimOnce = 1;    // value for number of loops argument for playing anim once
  const bool kCanAnimationInterrupt = true; // value for whether starting the animation can interrupt
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevPettingTestSimple::BehaviorDevPettingTestSimple(const Json::Value& config)
: ICozmoBehavior(config)
, _animTrigPetting()
, _timeTilTouchCheck(5.0f)
, _blissTimeout(5.0f)
, _nonBlissTimeout(5.0f)
, _minNumPetsToAdvanceBliss(2)
, _tracksToLock(0)
, _checkForTimeoutTimeBliss(std::numeric_limits<float>::max())
, _checkForTimeoutTimeNonbliss(std::numeric_limits<float>::max())
, _checkForTransitionTime(std::numeric_limits<float>::max())
, _currBlissLevel(0)
, _numPressesAtCurrentBlissLevel(0)
, _numTicksPressed(0)
, _isPressed(false)
, _reachedBliss(false)
, _isPressedPrevTick(false)
{
  const char* kDebugStr = "BehaviorDevPettingTestSimple.Ctor";
  
  _timeTilTouchCheck = JsonTools::ParseFloat( config, "timeTilTouchCheck", kDebugStr);
  
  std::vector<std::string> tmp;
  ANKI_VERIFY(JsonTools::GetVectorOptional(config, "animGroupNames", tmp),"BehaviorDevPettingTestSimple.Ctor.MissingVectorOfAnimTriggers","");
  for(const auto& t : tmp) {
    _animTrigPetting.push_back( AnimationTriggerFromString(t) );
  }
  
  _animTrigPettingGetin = AnimationTriggerFromString(JsonTools::ParseString(config, "animGroupGetin",kDebugStr));
  _animTrigPettingGetout = AnimationTriggerFromString(JsonTools::ParseString(config, "animGroupGetout",kDebugStr));
  
  bool animBodyTrackEnabled = JsonTools::ParseBool(config, "animBodyTrackEnabled",kDebugStr);
  bool animHeadTrackEnabled = JsonTools::ParseBool(config, "animHeadTrackEnabled",kDebugStr);
  bool animLiftTrackEnabled = JsonTools::ParseBool(config, "animLiftTrackEnabled",kDebugStr);
  
  // TODO(agm) dev feature to lock tracks while testing out different animation stubs for easier access to touch-sensor
  _tracksToLock = u8(animBodyTrackEnabled ? AnimTrackFlag::NO_TRACKS : AnimTrackFlag::BODY_TRACK) |
                  u8(animHeadTrackEnabled ? AnimTrackFlag::NO_TRACKS : AnimTrackFlag::HEAD_TRACK) |
                  u8(animLiftTrackEnabled ? AnimTrackFlag::NO_TRACKS : AnimTrackFlag::LIFT_TRACK);
  
  _blissTimeout = JsonTools::ParseFloat(config, "timeTilBlissGetout",kDebugStr);
  _nonBlissTimeout = JsonTools::ParseFloat(config, "timeTilNonBlissGetout",kDebugStr);
  _minNumPetsToAdvanceBliss = JsonTools::ParseInt8(config, "numPetsToAdvanceBliss",kDebugStr);
  
  SubscribeToTags({
    ExternalInterface::MessageEngineToGameTag::TouchButtonEvent
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPettingTestSimple::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  const EngineToGameTag& tag = event.GetData().GetTag();
  switch( tag )
  {
    case ExternalInterface::MessageEngineToGameTag::TouchButtonEvent:
    {
      _isPressed = event.GetData().Get_TouchButtonEvent().isPressed;
      if( _isPressed ) {
        auto touchTimePress = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        _checkForTransitionTime = touchTimePress + _timeTilTouchCheck;
        _numPressesAtCurrentBlissLevel++;
      } else {
        auto touchTimeRelease = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        _checkForTimeoutTimeBliss = touchTimeRelease + _blissTimeout;
        _checkForTimeoutTimeNonbliss = touchTimeRelease + _nonBlissTimeout;
      }
    }
      break;
    default: {
      PRINT_NAMED_ERROR("BehaviorDevPettingTestSimple.AlwaysHandle.InvalidEvent",
                        "%s", MessageEngineToGameTagToString(tag));
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevPettingTestSimple::WantsToBeActivatedBehavior() const
{
  return _isPressed;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPettingTestSimple::InitBehavior()
{
  _reachedBliss = false;
  _numPressesAtCurrentBlissLevel = 0;
  _currBlissLevel = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPettingTestSimple::OnBehaviorActivated()
{
  // GETIN
  TriggerAnimationAction* action = new TriggerAnimationAction( _animTrigPettingGetin, kPlayAnimOnce, kCanAnimationInterrupt, _tracksToLock);
  CancelAndPlayAction(action);
}

void BehaviorDevPettingTestSimple::AudioStateMachine(int ticksVolumeIncPeriod,
                                                     int volumeLevelInc) const
{
  using AMD_GE_GE = AudioMetaData::GameEvent::GenericEvent;
  using AMD_GOT = AudioMetaData::GameObjectType;
  
  // lambda helper to modulate volume increases
  const auto upVolume = [this,volumeLevelInc]() {
    for(int i=0; i<volumeLevelInc; ++i) {
      GetBEI().GetRobotAudioClient().PostEvent(AMD_GE_GE::Play__Robot_Vic_Sfx__Purr_Increase_Level, AMD_GOT::Cozmo_OnDevice);
    }
  };
  
  if (_numTicksPressed == 1) {
    // acknowledge touch
    GetBEI().GetRobotAudioClient().PostEvent(AMD_GE_GE::Play__Robot_Vic_Sfx__Purr_Loop_Play, AMD_GOT::Cozmo_OnDevice);
    upVolume();
  } else if ( (_numTicksPressed > 0) && (_numTicksPressed % ticksVolumeIncPeriod==0) ) {
    // increase loudness steadily
    upVolume();
  } else if ( !_isPressed && _isPressedPrevTick ) {
    // halt purring, fade out
    GetBEI().GetRobotAudioClient().PostEvent(AMD_GE_GE::Stop__Robot_Vic_Sfx__Purr_Loop_Stop, AMD_GOT::Cozmo_OnDevice);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPettingTestSimple::CancelAndPlayAction(TriggerAnimationAction* action, bool doCancelSelf)
{
  CancelDelegates();
  DelegateIfInControl(action,
                      [this,doCancelSelf](ActionResult result) {
                        if(doCancelSelf) {
                          CancelSelf();
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPettingTestSimple::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  const bool willBlissTimeout = now > _checkForTimeoutTimeBliss;
  const bool willNonBlissTimeout =  now > _checkForTimeoutTimeNonbliss;
  
  const bool willCheckForTransition = now > _checkForTransitionTime;
  
  if(_isPressed) {
    _numTicksPressed++;
  } else {
    _numTicksPressed = 0;
  }
  
  // NOTE: do not purr when playing getin
  if ((_currBlissLevel > 0) && (_currBlissLevel < _animTrigPetting.size()-1)) {
    // rising bliss petting mode sounds
    int audioVolUpTickPeriod = int(_animTrigPetting.size()-_currBlissLevel)*2;
    AudioStateMachine(audioVolUpTickPeriod, kAudioVolumeUpIncrementNonbliss);
  } else if( _currBlissLevel == _animTrigPetting.size()-1 ) {
    // blissful purring (when touched)
    AudioStateMachine(kAudioVolumeUpTickPeriodBliss, kAudioVolumeUpIncrementBliss); // faster and louder
  }
  
  if (((_reachedBliss && willBlissTimeout) || (!_reachedBliss && willNonBlissTimeout)) && !_isPressed)
  {
    // petting session should end (==> this behavior de-activates), but first, if bliss was
    // achieved, play an animation
    if( _reachedBliss ) {
      // prevent spamming this getout state transition once it has been started
      _checkForTimeoutTimeBliss = now + _blissTimeout;
      _checkForTimeoutTimeNonbliss = now + _nonBlissTimeout;
      
      // currently playing bliss loop
      // => play the "bliss" specific getout
      TriggerAnimationAction* action = new TriggerAnimationAction( _animTrigPettingGetout,
                                                                  kPlayAnimOnce,
                                                                  kCanAnimationInterrupt,
                                                                  _tracksToLock);
      CancelAndPlayAction(action, true);
    } else {
      CancelSelf();
    }
    
  } else if( (willCheckForTransition && _isPressed) || (_numPressesAtCurrentBlissLevel > _minNumPetsToAdvanceBliss) ) {
    // RISING BLISS
    
    // we are currently experiencing contact
    // => interrupt the current animation with the next sequential rising bliss anim
    // note: if we are at bliss do not interrupt
    
    if(_currBlissLevel == _animTrigPetting.size()-1) {
      if(!_reachedBliss) {
        // indicates we've played this already and we want to let it loop
        TriggerAnimationAction* action = new TriggerAnimationAction( _animTrigPetting[_currBlissLevel], kPlayAnimForever, kCanAnimationInterrupt, _tracksToLock);
        CancelAndPlayAction(action);
        _numPressesAtCurrentBlissLevel = 0;
        _reachedBliss = true;
      }
    } else {
      _checkForTransitionTime = now + _timeTilTouchCheck; // update the time to check for next transition, after each state transition
      TriggerAnimationAction* action = new TriggerAnimationAction( _animTrigPetting[_currBlissLevel],
                                                                  kPlayAnimOnce, kCanAnimationInterrupt, _tracksToLock);
      _currBlissLevel++;
      CancelAndPlayAction(action);
      _reachedBliss = false;
      _numPressesAtCurrentBlissLevel = 0;
    }
  }
  
  _isPressedPrevTick = _isPressed;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPettingTestSimple::OnBehaviorDeactivated()
{
  _numPressesAtCurrentBlissLevel = 0;
  _currBlissLevel = 0;
  _reachedBliss = false;
  _checkForTimeoutTimeNonbliss = std::numeric_limits<float>::max();
  _checkForTransitionTime = std::numeric_limits<float>::max();
  _checkForTimeoutTimeBliss = std::numeric_limits<float>::max();
}

} // Cozmo
} // Anki
