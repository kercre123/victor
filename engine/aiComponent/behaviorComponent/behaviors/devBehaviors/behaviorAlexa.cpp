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
  SubscribeToTags({ RobotInterface::RobotToEngineTag::alexaUXStateChanged });
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
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
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
  
  // don't do anything for now
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
                          AnimationTrigger::VC_ListeningGetIn,
                          postAudioEvent );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  if( _dVars.state == State::ListeningGetIn ) {
    if( !GetBehaviorComp<UserIntentComponent>().WaitingForTriggerWordGetInToFinish() ){
      if( _dVars.uxState != AlexaUXState::Listening ) {
        TransitionToListeningGetOut();
      } else {
        TransitionToListeningLoop();
      }
    }
  } else if( _dVars.state == State::ListeningLoop ) {
    if( _dVars.uxState != AlexaUXState::Listening ) {
      TransitionToListeningGetOut();
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::AlwaysHandleInScope(const RobotToEngineEvent& event)
{
  _dVars.uxState = event.GetData().Get_alexaUXStateChanged().state;
  PRINT_NAMED_WARNING("WHATNOW", "engine received ux state %d", (int)_dVars.uxState);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionToListeningLoop()
{
  CancelDelegates(false);
  _dVars.state = State::ListeningLoop;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::VC_ListeningLoop, 0 };
  DelegateIfInControl( action );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionToListeningGetOut()
{
  CancelDelegates(false);
  _dVars.state = State::ListeningGetOut_ToNothing;
  auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::VC_ListeningGetOut };
  DelegateIfInControl( action, [this](ActionResult res){ CancelSelf(); } );
}

}
}
