/**
 * File: BehaviorAlexa.cpp
 *
 * Author: ross
 * Created: 2018-10-05
 *
 * Description: plays animations for various alexa UX states
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorAlexa.h"
#include "engine/actions/animActions.h"
#include "audioEngine/multiplexer/audioCladMessageHelper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"

namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexa::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexa::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexa::BehaviorAlexa(const Json::Value& config)
 : ICozmoBehavior(config)
{
  SubscribeToTags(std::set<RobotInterface::RobotToEngineTag>{
    RobotInterface::RobotToEngineTag::alexaUXStateChanged,
    RobotInterface::RobotToEngineTag::alexaWeather,
  });
}

void BehaviorAlexa::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.dttb = BC.FindBehaviorByID(BEHAVIOR_ID(DanceToTheBeatCoordinator));
  _iConfig.weatherBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(WeatherResponses));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexa::~BehaviorAlexa()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAlexa::WantsToBeActivatedBehavior() const
{
  return _dVars.uxState != AlexaUXState::Idle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.dttb.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//    // TODO: insert any possible root-level json keys that this class is expecting.
//    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::OnBehaviorActivated() 
{
  const auto uxState = _dVars.uxState;
  // reset dynamic variables
  _dVars = DynamicVariables();
  _dVars.uxState = uxState;
  
  SmartDisableKeepFaceAlive();
  
  if( _dVars.uxState == AlexaUXState::Speaking ) {
    TransitionToSpeakingGetIn();
  }
}
  
void BehaviorAlexa::OnBehaviorEnteredActivatableScope()
{
  namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
  const auto postAudioEvent
    = AECH::CreatePostAudioEvent( AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On,
                                  AudioMetaData::GameObjectType::Behavior,
                                  0 );
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
  uic.SetResponseToAlexa( GetDebugLabel(),
                          AnimationTrigger::AlexaListenGetIn,
                          postAudioEvent );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  const bool shouldExitForWeather = (_dVars.shouldExitForWeatherWithinTicks > 0);
  if( shouldExitForWeather && _dVars.state != State::TransitioningToWeather ) {
    // give it a few ticks to wtba
    --_dVars.shouldExitForWeatherWithinTicks;
    if( _iConfig.weatherBehavior->WantsToBeActivated() ) {
      _dVars.shouldExitForWeatherWithinTicks = 0;
      _dVars.state = State::TransitioningToWeather;
      CancelDelegates(false);
      auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaToWeather };
      DelegateIfInControl(action, [this](const ActionResult& res) {
        CancelSelf();
      });
      return;
    }
  } else if( _dVars.state == State::TransitioningToWeather ) {
    _dVars.shouldExitForWeatherWithinTicks = 0;
    return;
  }
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool canDance = currTime_s >= _dVars.nextTimeCanDance_s;
  
  if( _iConfig.dttb->IsActivated() ) {
    if( _dVars.uxState == AlexaUXState::Idle ) {
      CancelDelegates(false);
    } else {
      return;
    }
  } else if( canDance && !_iConfig.dttb->IsActivated() && _iConfig.dttb->WantsToBeActivated() && _dVars.uxState != AlexaUXState::Idle ) {
    CancelDelegates(false);
    DelegateIfInControl( _iConfig.dttb.get(), [this]() {
      _dVars.nextTimeCanDance_s = 5.0f + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if( _dVars.uxState == AlexaUXState::Speaking ) {
        TransitionToSpeakingGetIn();
      } else if( _dVars.uxState == AlexaUXState::Listening ) {
        TransitionToListeningLoop(); // this should be the getin, but it's only in anim process for now
      } 
    });
    return;
  }
  
  if( _dVars.uxState == AlexaUXState::Idle ) {
    // could dance immediately next time
    _dVars.nextTimeCanDance_s = 0.0f;
  }
  
  switch( _dVars.state ) {
    case State::ListeningGetIn:
    {
      if( !GetBehaviorComp<UserIntentComponent>().WaitingForTriggerWordGetInToFinish() ){
        if( _dVars.uxState != AlexaUXState::Listening ) {
          TransitionToListeningGetOut();
        } else {
          TransitionToListeningLoop();
        }
      }
    }
      break;
    case State::ListeningLoop:
    {
      if( _dVars.uxState == AlexaUXState::Idle ) {
        TransitionToListeningGetOut();
      } else if( _dVars.uxState == AlexaUXState::Speaking ) {
        TransitionFromListeningToSpeaking();
      }
    }
      break;
    case State::SpeakingLoop:
    {
      if( _dVars.uxState == AlexaUXState::Idle ) {
        TransitionToSpeakingGetOut();
      } else if( _dVars.uxState == AlexaUXState::Listening ) {
        TransitionFromSpeakingToListening();
      }
    }
      break;
    case State::ListeningGetOut:
    case State::SpeakingGetIn:
    case State::SpeakingGetOut:
    case State::ListeningToSpeaking:
    case State::SpeakingToListening:
    case State::TransitioningToWeather:
      // each of these has a callback to transition to one of the states handled above, or CancelSelf()
      break;
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::CheckForExit()
{
  if( _dVars.lastReceivedWeather == 0 || _dVars.lastReceivedSpeak == 0 ) {
    return;
  }
  const int kMaxDelay = 80;
  PRINT_NAMED_WARNING("WHATNOW", "CheckForExit %d %d", _dVars.lastReceivedWeather, _dVars.lastReceivedSpeak);
  if( _dVars.lastReceivedWeather < _dVars.lastReceivedSpeak + kMaxDelay ) {
    _dVars.shouldExitForWeatherWithinTicks = 3;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::AlwaysHandleInScope(const RobotToEngineEvent& event)
{
  if( event.GetData().GetTag() == RobotInterface::RobotToEngineTag::alexaUXStateChanged ) {
    
    _dVars.lastReceivedSpeak = (int)BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    _dVars.uxState = event.GetData().Get_alexaUXStateChanged().state;
    CheckForExit();
    
    //PRINT_NAMED_WARNING("WHATNOW", "engine received ux state %d", (int)_dVars.uxState);
  } else if( event.GetData().GetTag() == RobotInterface::RobotToEngineTag::alexaWeather ) {
    //PRINT_NAMED_WARNING("WHATNOW", "engine received weather %s in state %d", event.GetData().Get_alexaWeather().condition.c_str(), (int)_dVars.uxState);
    _dVars.lastReceivedWeather = (int)BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    CheckForExit();
    
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionToListeningLoop()
{
  CancelDelegates(false);
  _dVars.state = State::ListeningLoop;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaListenLoop, 0 };
  action->SetRenderInEyeHue( false );
  DelegateIfInControl( action );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionToListeningGetOut()
{
  CancelDelegates(false);
  _dVars.state = State::ListeningGetOut;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaListenTimeout };
  action->SetRenderInEyeHue( false );
  DelegateIfInControl( action, [this](ActionResult res){ CancelSelf(); } );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionFromListeningToSpeaking()
{
  CancelDelegates(false);
  _dVars.state = State::ListeningToSpeaking;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaListen2Speak };
  action->SetRenderInEyeHue( false );
  DelegateIfInControl( action, [this](ActionResult res){
    TransitionToSpeakingLoop();
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionFromSpeakingToListening()
{
  CancelDelegates(false);
  _dVars.state = State::SpeakingToListening;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaSpeak2Listen };
  action->SetRenderInEyeHue( false );
  DelegateIfInControl( action, [this](ActionResult res){
    TransitionToListeningLoop();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionToSpeakingGetIn()
{
  CancelDelegates(false);
  _dVars.state = State::SpeakingGetIn;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaSuddenSpeak };
  action->SetRenderInEyeHue( false );
  DelegateIfInControl( action, [this](ActionResult res){
    TransitionToSpeakingLoop();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionToSpeakingLoop()
{
  CancelDelegates(false);
  _dVars.state = State::SpeakingLoop;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaSpeakLoop, 0 };
  action->SetRenderInEyeHue( false );
  DelegateIfInControl( action );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionToSpeakingGetOut()
{
  CancelDelegates(false);
  _dVars.state = State::SpeakingGetOut;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::AlexaSpeakGetOut };
  action->SetRenderInEyeHue( false );
  DelegateIfInControl( action, [this](ActionResult res){
    CancelSelf();
  });
}

}
}
