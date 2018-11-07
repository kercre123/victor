/**
 * File: behaviorAlexa.cpp
 *
 * Author: ross
 * Created: 2018-11-01
 *
 * Description: Plays animations that mirror Alexa UX state
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/alexa/behaviorAlexa.h"
#include "clad/types/animationTrigger.h"
#include "engine/aiComponent/alexaComponent.h"

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexa::~BehaviorAlexa()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAlexa::WantsToBeActivatedBehavior() const
{
  const auto& alexaComp = GetAIComp<AlexaComponent>();
  const bool isIdle = alexaComp.IsIdle();
  return !isIdle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::OnBehaviorEnteredActivatableScope()
{
  auto& alexaComp = GetAIComp<AlexaComponent>();
  
  std::unordered_map<AlexaUXState, AlexaComponent::AlexaUXResponse> responses;
  // for now, just play the wake word sound effect when it goes from idle -> non-idle
  responses[AlexaUXState::Listening].animTrigger = AnimationTrigger::InvalidAnimTrigger;
  responses[AlexaUXState::Listening].audioEvent = AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On;
  responses[AlexaUXState::Speaking].animTrigger = AnimationTrigger::InvalidAnimTrigger;
  responses[AlexaUXState::Speaking].audioEvent = AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On;
  responses[AlexaUXState::Thinking].animTrigger = AnimationTrigger::InvalidAnimTrigger;
  responses[AlexaUXState::Thinking].audioEvent = AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On;
  
  alexaComp.SetAlexaUXResponses( responses );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::OnBehaviorLeftActivatableScope()
{
  auto& alexaComp = GetAIComp<AlexaComponent>();
  alexaComp.ResetAlexaUXResponses();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  const auto& alexaComp = GetAIComp<AlexaComponent>();
  if( alexaComp.IsIdle() ) {
    // we might consider a timeout here if the ux state switches very briefly to idle
    CancelSelf();
  }
}

}
}
