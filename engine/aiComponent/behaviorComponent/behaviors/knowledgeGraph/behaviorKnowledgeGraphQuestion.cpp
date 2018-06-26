/***********************************************************************************************************************
 *
 *  BehaviorLeaveAMessage
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 4/01/2018
 *
 *  Description
 *  + Parent behavior for allowing the user to record a message and save it to Victor's local storage.
 *
 **********************************************************************************************************************/

// #include "engine/aiComponent/behaviorComponent/behaviors/knowledgeGraph/behaviorKnowledgeGraphQuestion.h"
#include "behaviorKnowledgeGraphQuestion.h"
#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/mics/micComponent.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/behaviorComponent/userIntent.h"

#define PRINT_DEBUG( format, ... ) \
  PRINT_CH_DEBUG( "KnowledgeGraph", "BehaviorKnowledgeGraphQuestion", format, ##__VA_ARGS__ )

#define PRINT_INFO( format, ... ) \
  PRINT_CH_INFO( "KnowledgeGraph", "BehaviorKnowledgeGraphQuestion", format, ##__VA_ARGS__ )

// notes:
// + need to check if Houndify connection is valid and respond accordingly if it is not
// * move "interruptions" into BehaviorTextToSpeechLoop since it's common use case
// * use BehaviorPromptUserForVoiceCommand for the question/response

namespace Anki {
namespace Cozmo {

namespace
{
  const char* kKey_Duration                       = "streamingTimeout";
  const float kDefaultDuration                    = 10.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::InstanceConfig::InstanceConfig() :
  streamingDuration( kDefaultDuration )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::DynamicVariables::DynamicVariables() :
  state( EState::TransitionToListening ),
  ttsGenerationStatus( EGenerationStatus::None )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::BehaviorKnowledgeGraphQuestion( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  JsonTools::GetValueOptional( config, kKey_Duration, _iVars.streamingDuration );

  SubscribeToTags({{
    ExternalInterface::MessageEngineToGameTag::TouchButtonEvent,
    ExternalInterface::MessageEngineToGameTag::RobotFallingEvent, // do we need this?
  }});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const
{
  expectedKeys.insert( kKey_Duration );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::InitBehavior()
{
  const BehaviorContainer& container = GetBEI().GetBehaviorContainer();
  container.FindBehaviorByIDAndDowncast<BehaviorTextToSpeechLoop>( BEHAVIOR_ID(KnowledgeGraphTTS),
                                                                   BEHAVIOR_CLASS(TextToSpeechLoop),
                                                                   _iVars.ttsBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  delegates.insert( _iVars.ttsBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = false;
  modifiers.behaviorAlwaysDelegates               = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorKnowledgeGraphQuestion::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::OnBehaviorActivated()
{
  _dVars = DynamicVariables();

  // open up streaming after we play our get-in to avoid motor noise
  DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphGetIn ),
                       &BehaviorKnowledgeGraphQuestion::BeginStreamingQuestion );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::OnBehaviorDeactivated()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::HandleWhileActivated( const EngineToGameEvent& event )
{
  using namespace ExternalInterface;

  const MessageEngineToGameTag tag = event.GetData().GetTag();
  switch ( tag )
  {
    case MessageEngineToGameTag::TouchButtonEvent:
    case MessageEngineToGameTag::RobotFallingEvent:
    {
      if ( EState::Responding == _dVars.state )
      {
        OnResponseInterrupted();
      }
      break;
    }

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::BehaviorUpdate()
{
  if ( IsActivated() )
  {
    UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
    const bool triggerWordPending = uic.IsTriggerWordPending();

    // we should clear this each time it's pending regardless of state so the UIC doesn't complain about it not being handled
    if ( triggerWordPending )
    {
      uic.ClearPendingTriggerWord();
    }

    // if we're recording the user's question, we need to be listening for a response
    if ( EState::Listening == _dVars.state )
    {
      // if we've gotten a response from knowledge graph, transition into the response state
      if ( IsResponsePending() )
      {
        CancelDelegates( false );
        OnStreamingComplete();
      }
    }
    else if ( EState::Responding == _dVars.state )
    {
      // if we're playing our response, allow victor to be interrupted
      const bool isPickedUp = GetBEI().GetRobotInfo().IsPickedUp();
      if ( isPickedUp || triggerWordPending )
      {
        OnResponseInterrupted();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::BeginStreamingQuestion()
{
  PRINT_DEBUG( "Knowledge Graph streaming begun ..." );

  GetBEI().GetMicComponent().StartWakeWordlessStreaming( CloudMic::StreamType::KnowledgeGraph );
  _dVars.state = EState::Listening;

  // loop the listening animation for the duration of our stream
  TriggerAnimationAction* listeningAnimationAction =
    new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphListening,
                                0, // loop forever
                                true,
                                static_cast<uint8_t>(AnimTrackFlag::NO_TRACKS),
                                _iVars.streamingDuration ); // timeout after this duration

  DelegateIfInControl( listeningAnimationAction, &BehaviorKnowledgeGraphQuestion::OnStreamingComplete );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::OnStreamingComplete()
{
  // go into responding state first, then figure out how to reponde
  _dVars.state = EState::Searching;

  // see if we got a response from knowledge graph
  if ( IsResponsePending() )
  {
    // need to consume the response immediately or the system gets cranky
    ConsumeResponse();
  }

  // did we get a response from knowledge graph?
  if ( !_dVars.responseString.empty() )
  {
    PRINT_INFO( "Streaming is complete ... valid response received" );

    // start generating the response now so that we can minimize the wait time
    auto callback = [this]( bool success )
    {
      if ( success )
      {
        PRINT_INFO( "TTS generation is complete" );
        _dVars.ttsGenerationStatus = EGenerationStatus::Success;
      }
      else
      {
        PRINT_INFO( "TTS generation FAILED" );
        _dVars.ttsGenerationStatus = EGenerationStatus::Fail;
      }
    };

    _iVars.ttsBehavior->SetTextToSay( _dVars.responseString, callback );

    // let's transition into our "searching" loop
    // since we always want to loop at least once, play the get in + loop anim before we do any logic for the tts
    CompoundActionSequential* messageAnimation = new CompoundActionSequential();
    messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphSearchingGetIn ), true );
    messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphSearching ), true );

    DelegateIfInControl( messageAnimation,
                         &BehaviorKnowledgeGraphQuestion::TransitionToSearchingLoop );
  }
  else
  {
    PRINT_INFO( "Streaming is complete ... NO response received" );

    // if not, let the user know we failed ...
    TransitionToNoResponse();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorKnowledgeGraphQuestion::IsResponsePending() const
{
  const UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
  return uic.IsAnyUserIntentPending();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::ConsumeResponse()
{
  PRINT_DEBUG( "Checking for response" );

  // if we have a valid knowledge graph response intent, grab the response string from it
  // if it's not the knowledge graph response, we simply return the empty string which will be handled appropriately
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();

  UserIntent intent;
  if ( uic.IsUserIntentPending( USER_INTENT(knowledge_response), intent ) )
  {
    uic.DropUserIntent( intent.GetTag() );

    // grab the response string
    const UserIntent_KnowledgeResponse& intentResponse = intent.Get_knowledge_response();
    _dVars.responseString = intentResponse.answer;

    PRINT_DEBUG( "Knowledge Graph Question: %s", Util::HidePersonallyIdentifiableInfo( intentResponse.query_text.c_str() ) );
    PRINT_DEBUG( "Knowledge Graph Response: %s", Util::HidePersonallyIdentifiableInfo( intentResponse.answer.c_str() ) );
  }
  else if ( uic.IsUserIntentPending( USER_INTENT(knowledge_unknown) ) )
  {
    // don't need to do anything other than clear the response
    uic.DropUserIntent( USER_INTENT(knowledge_unknown) );
  }
  else if ( uic.IsUserIntentPending( USER_INTENT(unmatched_intent) ) )
  {
    // this shoudln't really happen, but handle it safely regardless
    uic.DropUserIntent( USER_INTENT(unmatched_intent) );
    PRINT_NAMED_WARNING( "BehaviorKnowledgeGraphQuestion", "unmatched_intent returned as response from knowledge graph" );
  }
  else
  {
    #if ALLOW_DEBUG_LOGGING
    {
      // this really shouldn't happen, something went wrong
      // this will be handled fine, but we should let dev know about it
      const UserIntentData* intentData = uic.GetPendingUserIntent();
      const std::string intentString = UserIntentTagToString( intentData ? intentData->intent.GetTag() : UserIntentTag::INVALID );
      PRINT_NAMED_ERROR( "BehaviorKnowledgeGraphQuestion", "Invalid intent returned from Knowledge Graph: %s", intentString.c_str() );
    }
    #endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::BeginResponseTTS()
{
  PRINT_DEBUG( "Starting TTS behavior ..." );

  // delegate to our tts behavior
  if ( _iVars.ttsBehavior->WantsToBeActivated() )
  {
    DelegateIfInControl( _iVars.ttsBehavior.get() );
  }

  // ... annnnnd we're done
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToSearchingLoop()
{
  // keep looping back here until the tts audio has been generated ...
  if ( EGenerationStatus::None != _dVars.ttsGenerationStatus )
  {
    if ( EGenerationStatus::Success == _dVars.ttsGenerationStatus )
    {
      // TTS generation is done, so let's speak it!
      TransitionToBeginResponse();
    }
    else
    {
      // we failed to generate the tts, but since we've already looped our searching anim, we just need to get out now
      DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphSearchingGetOutFail ) );

      // ... annnnnd we're done
    }
  }
  else
  {
    DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphSearching ),
                         &BehaviorKnowledgeGraphQuestion::TransitionToSearchingLoop );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToBeginResponse()
{
  _dVars.state = EState::Responding;

  DEV_ASSERT( !_dVars.responseString.empty(), "Responding to knowledge graph request but no response string exists" );

  // nothing to do but speak the response
  BeginResponseTTS();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToNoResponse()
{
  _dVars.state = EState::NoResponse;

  // our get-out from listening is simply a series of animations
  CompoundActionSequential* messageAnimation = new CompoundActionSequential();
  messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphSearchingGetIn ), true );
  messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphSearching ), true );
  messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::KnowledgeGraphSearchingGetOutFail ), true );

  DelegateIfInControl( messageAnimation );

  // ... annnnnd we're done
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToNoConnection()
{
  PRINT_DEBUG( "Knowledge Graph not connected" );

  _dVars.state = EState::NoConnection;

  // currently no distinction between "no reponse", but there will/may be at some point
  TransitionToNoResponse();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::OnResponseInterrupted()
{
  DEV_ASSERT( EState::Responding == _dVars.state, "Should only allow interruptions during response state" );
  PRINT_INFO( "Interruption event received, cancelling TTS" );

  _dVars.state = EState::Interrupted;
  if ( IsControlDelegated() && _iVars.ttsBehavior.get()->IsActivated() )
  {
    _iVars.ttsBehavior.get()->Interrupt( false );
  }
}

} // namespace Cozmo
} // namespace Anki

#undef PRINT_DEBUG

