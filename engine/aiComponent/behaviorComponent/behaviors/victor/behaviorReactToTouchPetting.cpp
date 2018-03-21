/**
 * File: BehaviorReactToTouchPetting.cpp
 *
 * Author: Arjun Menon
 * Date:   09/12/2017
 *
 * Description: The robot's core reactions to repeated touches in the form of
 *              petting.
 *              This is embodied by increasing levels of bliss, with a getout
 *              for each level, and a final bliss cycle that loops forever until
 *              no more incoming touches
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorReactToTouchPetting.h"

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
  // animation control constants
  const int kPlayAnimOnce = 1;    // value for number of loops argument for playing anim once
  const bool kCanAnimationInterrupt = true; // value for whether starting the animation can interrupt
  
  const char* const kTimeTilTouchCheckKey = "timeTilTouchCheck";
  const char* const kAnimGroupNamesKey = "animGroupNames";
  const char* const kAnimGroupNamesGetoutKey = "animGroupNamesGetout";
  const char* const kTimeTilBlissGetoutKey = "timeTilBlissGetout";
  const char* const kTimeTilNonBlissGetoutKey = "timeTilNonBlissGetout";
  const char* const kNumPetsToAdvanceBlissKey = "numPetsToAdvanceBliss";
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToTouchPetting::BehaviorReactToTouchPetting(const Json::Value& config)
: ICozmoBehavior(config)
, _animPettingResponse()
, _animPettingGetout()
, _timeTilTouchCheck(5.0f)
, _blissTimeout(5.0f)
, _nonBlissTimeout(5.0f)
, _minNumPetsToAdvanceBliss(2)
, _currResponseState(Done)
, _checkForTimeoutTimeBliss(std::numeric_limits<float>::max())
, _checkForTimeoutTimeNonbliss(std::numeric_limits<float>::max())
, _checkForTransitionTime(std::numeric_limits<float>::max())
, _currBlissLevel(0)
, _numPressesAtCurrentBlissLevel(0)
, _numTicksPressed(0)
, _isPressed(false)
, _isPressedPrevTick(false)
{
  const char* kDebugStr = "BehaviorReactToTouchPetting.Ctor";
  
  _timeTilTouchCheck = JsonTools::ParseFloat( config, kTimeTilTouchCheckKey, kDebugStr);
  
  std::vector<std::string> tmp;
  ANKI_VERIFY(JsonTools::GetVectorOptional(config, kAnimGroupNamesKey, tmp),"BehaviorReactToTouchPetting.Ctor.MissingVectorOfAnimTriggers","");
  for(const auto& t : tmp) {
    _animPettingResponse.push_back( AnimationTriggerFromString(t) );
  }
  
  tmp.clear();
  ANKI_VERIFY(JsonTools::GetVectorOptional(config, kAnimGroupNamesGetoutKey, tmp),"BehaviorReactToTouchPetting.Ctor.MissingVectorOfAnimTriggersGetout","");
  for(const auto& t : tmp) {
    _animPettingGetout.push_back( AnimationTriggerFromString(t) );
  }
  
  _blissTimeout = JsonTools::ParseFloat(config, kTimeTilBlissGetoutKey,kDebugStr);
  _nonBlissTimeout = JsonTools::ParseFloat(config, kTimeTilNonBlissGetoutKey,kDebugStr);
  _minNumPetsToAdvanceBliss = JsonTools::ParseInt8(config, kNumPetsToAdvanceBlissKey,kDebugStr);
  
  SubscribeToTags({
    ExternalInterface::MessageEngineToGameTag::TouchButtonEvent
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTouchPetting::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
     kTimeTilTouchCheckKey,
     kAnimGroupNamesKey,
     kAnimGroupNamesGetoutKey,
     kTimeTilBlissGetoutKey,
     kTimeTilNonBlissGetoutKey,
     kNumPetsToAdvanceBlissKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTouchPetting::AlwaysHandleInScope(const EngineToGameEvent& event)
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
      PRINT_NAMED_ERROR("BehaviorReactToTouchPetting.AlwaysHandle.InvalidEvent",
                        "%s", MessageEngineToGameTagToString(tag));
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToTouchPetting::WantsToBeActivatedBehavior() const
{
  return _isPressed;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTouchPetting::InitBehavior()
{
  _numPressesAtCurrentBlissLevel = 0;
  _currBlissLevel = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTouchPetting::OnBehaviorActivated()
{
  // starts the state machine to check for updates
  _currResponseState = PettingResponseState::PlayTransitionToLevel;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTouchPetting::CancelAndPlayAnimation(AnimationTrigger anim)
{
  TriggerAnimationAction* action = new TriggerAnimationAction(anim,
                                                              kPlayAnimOnce,
                                                              kCanAnimationInterrupt);
  CancelDelegates();
  DelegateIfInControl(action);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTouchPetting::PlayBlissLoopAnimation()
{
  TriggerAnimationAction* action = new TriggerAnimationAction(_animPettingResponse.back(),
                                                              kPlayAnimOnce,
                                                              kCanAnimationInterrupt);
  
  ActionResultCallback cb = [this](ActionResult result)->void {
    // note: since Update() waits for animations to finish before
    //       playing any corresponding getout, animation is not
    //       re-queued if a getout is requested
    if( this->_currResponseState!=PlayGetoutFromLevel ) {
      this->PlayBlissLoopAnimation();
    }
  };
  
  DelegateIfInControl(action, cb);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTouchPetting::BehaviorUpdate()
{
  using AMD_GE_GE = AudioMetaData::GameEvent::GenericEvent;
  using AMD_GOT = AudioMetaData::GameObjectType;
  
  if(!IsActivated()){
    return;
  }
  
  //-----------------------------------------------------------------------------
  // update the transition trigger variables
  const float now            = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool reachedMaxBliss = (_currBlissLevel == _animPettingResponse.size());
  if(_isPressed) {
    _numTicksPressed++;
  } else {
    _numTicksPressed = 0;
  }
  
  //-----------------------------------------------------------------------------
  // state machine for the petting response
  switch(_currResponseState) {
    case PlayTransitionToLevel:
    {
      const bool willBlissTimeout    = reachedMaxBliss  && (now > _checkForTimeoutTimeBliss);
      const bool willNonblissTimeout = !reachedMaxBliss && (now > _checkForTimeoutTimeNonbliss);
      const bool willTransition      = (_isPressed && (now > _checkForTransitionTime)) ||
                                       (_numPressesAtCurrentBlissLevel > _minNumPetsToAdvanceBliss);
      if (!_isPressed && (willBlissTimeout || willNonblissTimeout)) {
        // a Getout is requested; it will be played in the following tick
        _currResponseState            = PlayGetoutFromLevel;
        _checkForTimeoutTimeBliss     = now + _blissTimeout;
        _checkForTimeoutTimeNonbliss  = now + _nonBlissTimeout;
      } else if (!reachedMaxBliss && willTransition) {
        _currResponseState = PlayTransitionToLevel;
        // after the transition, we reset the conditions
        // for triggering the next transition
        _checkForTransitionTime = now + _timeTilTouchCheck;
        _numPressesAtCurrentBlissLevel = 0;

        // play the appropriate animation for the level
        const bool willTransitionToMaxBliss = _currBlissLevel==(_animPettingResponse.size()-1);
        if(willTransitionToMaxBliss) {
          // note: this method handles cycling through
          //       all variations of max bliss animation
          PlayBlissLoopAnimation();
        } else {
          const auto animIdx = _currBlissLevel;
          CancelAndPlayAnimation(_animPettingResponse[animIdx]);
        }
        
        // audio for petting purring
        if(_currBlissLevel==0) {
          GetBEI().GetRobotAudioClient().PostEvent(AMD_GE_GE::Play__Robot_Vic_Sfx__Purr_Loop_Play, AMD_GOT::Cozmo_OnDevice);
        } else {
          GetBEI().GetRobotAudioClient().PostEvent(AMD_GE_GE::Play__Robot_Vic_Sfx__Purr_Increase_Level, AMD_GOT::Cozmo_OnDevice);
        }
        
        // important: increase the bliss level AFTER getting the animation index
        // reasoning:
        // - value of 0 for _currBlissLevel implies only the getin has been played
        // - values 1-N for _currBlissLevel are for indexing the correct animation for the level
        // thus the animation's index is _currBlissLevel-1
        // this keeps it consistent with how we pick the getout animation index
        _currBlissLevel = MIN(_currBlissLevel+1, (int)_animPettingResponse.size());
      }
      break;
    }
    case PlayGetoutFromLevel:
    {
      // - stay in this state, until a getout can be played
      // - when a getout is played, transition to Done
      // - after the animation finishes, the behavior will
      //   automatically deactivate
      const bool wasPlayingGetin = _currBlissLevel == 0;
      if(wasPlayingGetin) {
        // previously in PlayGetin, currently in PlayGetoutFromLevel
        // no corresponding getout animation exists for this state
        CancelSelf();
      } else {
        // - previously was in PlayTransitionToLevel, now play appropriate getout
        // - expected flow:
        //      Update()      | entered bliss (in PlayTransitionToLevel)
        //                    |
        //      PlayBlissLoop | bliss animation plays and checks to requeue
        //                    |
        //      Update()      | getout requested (in PlayGetoutFromLevel)
        //                    | cannot play getout because animation is playing
        //                    |
        //      PlayBlissLoop | bliss animation cannot requeue because getout requested
        //                    |
        //      Update()      | Update() no animation playing, getout plays
        if(!IsControlDelegated()) {
          const auto animIdx = _currBlissLevel-1; // bliss levels are +1 wrt animation indexes
          CancelAndPlayAnimation(_animPettingGetout[animIdx]);
          
          
          GetBEI().GetRobotAudioClient().PostEvent(AMD_GE_GE::Stop__Robot_Vic_Sfx__Purr_Loop_Stop, AMD_GOT::Cozmo_OnDevice);
          
          // ensures we wait for the getout to finish before ending the behavior
          _currResponseState = Done;
        }
      }
      break;
    }
    case Done:
    {
      // - initial inactive state for petting
      // - also terminal state when waiting for getouts to finish
      // - when getout animation is finished, we deactivate the behavior
      if(!IsControlDelegated()) {
        CancelSelf();
      }
      break;
    }
  }
  
  _isPressedPrevTick = _isPressed;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTouchPetting::OnBehaviorDeactivated()
{
  _currResponseState              = Done;
  _numPressesAtCurrentBlissLevel  = 0;
  _currBlissLevel                 = 0;
  _checkForTimeoutTimeNonbliss    = std::numeric_limits<float>::max();
  _checkForTransitionTime         = std::numeric_limits<float>::max();
  _checkForTimeoutTimeBliss       = std::numeric_limits<float>::max();
}

} // Cozmo
} // Anki
