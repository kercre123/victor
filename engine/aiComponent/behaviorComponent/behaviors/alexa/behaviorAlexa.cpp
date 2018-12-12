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
#include "engine/actions/animActions.h"
#include "engine/aiComponent/alexaComponent.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* const kResponseDataKey = "responseData";

  #define SET_ANIM_STATE(s) do{ \
    _dVars.animState = AnimState::s; \
    PRINT_CH_INFO("Behaviors", "BehaviorAlexa.AnimState", "AnimState = %s", #s); \
  } while(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexa::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexa::DynamicVariables::DynamicVariables()
{
  animState = AnimState::Idle;
  uxState = AlexaUXState::Idle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAlexa::BehaviorAlexa(const Json::Value& config)
 : ICozmoBehavior(config)
{
  const auto& responseData = config[kResponseDataKey];
  if( ANKI_VERIFY(!responseData.isNull(), "BehaviorAlexa.Ctor.MissingResponseData", "Could not find key 'responseData'" ) ) {
    LoadConfig( responseData );
  }
}
     
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 void BehaviorAlexa::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
 {
   expectedKeys.insert( kResponseDataKey );
 }
     
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::LoadConfig( const Json::Value& data )
{
  
  _iConfig.uxResponses.clear();
  _iConfig.animInfoMap.clear();
  
  for( const auto& uxStateStr : data.getMemberNames() ) {
    AlexaUXState uxState;
    if( ANKI_VERIFY( AlexaUXStateFromString( uxStateStr, uxState),
                     "BehaviorAlexa.LoadConfig.InvalidUXState",
                     "'%s' is not a valid AlexaUXState",
                     uxStateStr.c_str() ) )
    {
      // load ux get-in animations
      const auto& stateInfo = data[uxStateStr];
      const auto& fromIdle = stateInfo["fromIdle"];
      if( !fromIdle.isNull() ) {
        std::string animTrigger;
        std::string audioEvent;
        if( fromIdle["animTrigger"].isString() ) {
          animTrigger = fromIdle["animTrigger"].asString();
        }
        if( fromIdle["audioEvent"].isString() ) {
          audioEvent = fromIdle["audioEvent"].asString();
        }
        if( !animTrigger.empty() ) {
          _iConfig.uxResponses[uxState].animTrigger = AnimationTriggerFromString( animTrigger );
        }
        if( !audioEvent.empty() ) {
          _iConfig.uxResponses[uxState].audioEvent = AudioMetaData::GameEvent::GenericEventFromString( audioEvent );
        }
      }
      
      // load transitions to other states
      if( ANKI_VERIFY( !stateInfo["transitions"].isNull(),
                        "BehaviorAlexa.LoadConfig.NoTransitions",
                        "'transitions' not found for ux state %s",
                        uxStateStr.c_str() ) )
      {
        const auto& transitions = stateInfo["transitions"];
        for( const auto& nextUXStateStr : transitions.getMemberNames() ) {
          AlexaUXState nextUXState;
          if( ANKI_VERIFY( AlexaUXStateFromString( nextUXStateStr, nextUXState),
                          "BehaviorAlexa.LoadConfig.InvalidNextUXState",
                          "'%s' is not a valid AlexaUXState",
                          nextUXStateStr.c_str() ) )
          {
            if( ANKI_VERIFY( transitions[nextUXStateStr].isString(),
                             "BehaviorAlexa.LoadConfig.InvalidTransAnim",
                             "'%s' does not list an anim trigger",
                             nextUXStateStr.c_str() ) )
            {
              const std::string& transAnim = transitions[nextUXStateStr].asString();
              AnimationTrigger transTrigger;
              if( ANKI_VERIFY( AnimationTriggerFromString( transAnim, transTrigger ),
                               "BehaviorAlexa.LoadConfig.InvalidTransTrigger",
                               "Could not read transition anim trigger from '%s'",
                               transAnim.c_str() ) )
              {
                // insert into map of what animation to play when transitioning from uxState to nextUXState
                _iConfig.animInfoMap[uxState].transitions.emplace_back( nextUXState, transTrigger );
              }
            }
          }
        }
      }
      
      // load loops
      if( ANKI_VERIFY( stateInfo["loop"].isString(),
                      "BehaviorAlexa.LoadConfig.NoLoops",
                      "'loop' not found for ux state %s",
                      uxStateStr.c_str() ) )
      {
        const std::string& loopAnim = stateInfo["loop"].asString();
        AnimationTrigger loopTrigger;
        if( ANKI_VERIFY( AnimationTriggerFromString( loopAnim, loopTrigger ),
                        "BehaviorAlexa.LoadConfig.InvalidLoop",
                        "Could not read loop anim trigger from '%s'",
                        loopAnim.c_str() ) )
        {
          // insert into map of what animation to play when transitioning from uxState to Idle
          _iConfig.animInfoMap[uxState].loop = loopTrigger;
        }
      }
    }
  }
  
  
  
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
  // always run
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;

  modifiers.behaviorAlwaysDelegates = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::OnBehaviorEnteredActivatableScope()
{
  auto& alexaComp = GetAIComp<AlexaComponent>();
  alexaComp.SetAlexaUXResponses( _iConfig.uxResponses );
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
  
  SmartDisableKeepFaceAlive();
  
  auto& alexaComp = GetAIComp<AlexaComponent>();
  const auto currUxState = alexaComp.GetUXState();
  
  if( !alexaComp.IsUXStateGetInPlaying( currUxState ) ) {
    // there was no get-in from idle for this state, or it somehow finished really quickly
    TransitionTo_StateLoop( currUxState );
  } else {
    // there is a get-in from idle for this state
    TransitionTo_IdleToNonIdle( currUxState );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  // TODO: to deal with eye color changes, we will need to replace the existing get-in anims with
  // anims for the current eye color whenever the setting changes
  
  const auto& alexaComp = GetAIComp<AlexaComponent>();
  
  const auto oldState = _dVars.uxState;
  const auto newState = alexaComp.GetUXState();
  const bool uxChanged = oldState != newState;
  // don't actually change _dVars.uxState. That only happens when starting an animation
  const bool animsComplete = !IsControlDelegated() && !alexaComp.IsAnyUXStateGetInPlaying();
  
  
  if( uxChanged ) {
    switch( _dVars.animState ) {
      case AnimState::StateLoop:
      {
        // transition immediately
        TransitionTo_Transition( oldState, newState );
      }
        break;
      case AnimState::IdleToNonIdle:
      case AnimState::Transition:
        // wait for any transition to complete, then start the next transition
        if( animsComplete && !alexaComp.IsAnyUXStateGetInPlaying() ) {
          TransitionTo_Transition( oldState, newState );
        }
        break;
      case AnimState::Idle:
      {
        DEV_ASSERT(false && "Unexpected state", "BehaviorAlexa.BehaviorUpdate.UnexpectedAnimState");
      }
        break;
    }
  } else if( animsComplete && !alexaComp.IsIdle() ) {
    // ux state didnt change, and any animations we have started have finished, which means we should now start
    // the state loop
    TransitionTo_StateLoop( newState );
  }
  
  if( alexaComp.IsIdle() && (_dVars.uxState == AlexaUXState::Idle) && animsComplete && !IsControlDelegated() ) {
    // we might consider a timeout here if the ux state switches very briefly to idle
    CancelSelf();
  }
}
     
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorAlexa::GetTransitionAnimation( AlexaUXState fromState, AlexaUXState toState ) const
{
  AnimationTrigger result = AnimationTrigger::Count;
  const auto it = _iConfig.animInfoMap.find( fromState );
  
  if( it != _iConfig.animInfoMap.end() ) {
    auto itTrigger = std::find_if( it->second.transitions.begin(), it->second.transitions.end(), [&](const auto& p){
      return p.first == toState;
    });
    if( itTrigger != it->second.transitions.end() ) {
      result = itTrigger->second;
    }
  }
  
  if( result == AnimationTrigger::Count ) {
    PRINT_NAMED_WARNING( "BehaviorAlexa.GetTransitionAnimation.NotFound",
                         "Could not find transition from '%s' to '%s'",
                         AlexaUXStateToString(fromState), AlexaUXStateToString(toState) );
  }
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorAlexa::GetLoopAnimation( AlexaUXState state ) const
{
  AnimationTrigger result = AnimationTrigger::Count;
  auto it = _iConfig.animInfoMap.find( state );
  
  if( it != _iConfig.animInfoMap.end() ) {
    result = it->second.loop;
  }
  
  if( result == AnimationTrigger::Count ) {
    PRINT_NAMED_WARNING( "BehaviorAlexa.GetLoopAnimation.NotFound",
                         "Could not loop for state '%s'",
                         AlexaUXStateToString(state) );
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionTo_IdleToNonIdle( AlexaUXState newState )
{
  SET_ANIM_STATE( IdleToNonIdle );
  _dVars.uxState = newState;
  
  // nothing to do here other than wait. the anim process is playing any getins we
  // provided in OnBehaviorEnteredActivatableScope
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionTo_StateLoop( AlexaUXState newState )
{
  SET_ANIM_STATE( StateLoop );
  _dVars.uxState = newState;
  
  const auto loopAnim = GetLoopAnimation( newState );
  if( loopAnim != AnimationTrigger::Count ) {
    auto* action = new TriggerLiftSafeAnimationAction{ loopAnim, 0 };
    action->SetRenderInEyeHue( false );
    DelegateIfInControl( action ); // no callback, Update() will transition out when the ux state changes
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAlexa::TransitionTo_Transition( AlexaUXState oldState, AlexaUXState newState )
{
  SET_ANIM_STATE( Transition );
  _dVars.uxState = newState;
  
  CancelDelegates(false);
  
  const auto transAnim = GetTransitionAnimation( oldState, newState );
  if( transAnim != AnimationTrigger::Count ) {
    auto* action = new TriggerLiftSafeAnimationAction{ transAnim };
    action->SetRenderInEyeHue( false );
    // no callback, Update() will either transition to the state loop, or, if the ux
    // state changes between now and when this action finishes, then another transition will run
    DelegateIfInControl( action );
  }
}
  
  
#undef SET_ANIM_STATE
}
}

