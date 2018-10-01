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
#include "clad/externalInterface/messageEngineToGame.h"
#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/audio/engineRobotAudioClient.h"
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
namespace Vector {

namespace
{
  const char* kKey_Duration                       = "streamingTimeout";
  const char* kKey_ReadyString                    = "readyPromptText";
  const char* kKey_EarConEnd                      = "earConAudioEventEnd";

  const double kDefaultDuration                   = 10.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::InstanceConfig::InstanceConfig() :
  streamingDuration( kDefaultDuration ),
  earConEnd( AudioMetaData::GameEvent::GenericEvent::Invalid )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::DynamicVariables::DynamicVariables() :
  state( EState::GettingIn ),
  streamingBeginTime( 0.0 ),
  ttsGenerationStatus( EGenerationStatus::None ),
  wasPickedUp( false )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::BehaviorKnowledgeGraphQuestion( const Json::Value& config ) :
  ICozmoBehavior( config ),
  _readyTTSWrapper( UtteranceTriggerType::Immediate )
{
  JsonTools::GetValueOptional( config, kKey_Duration, _iVars.streamingDuration );
  _iVars.readyText = JsonTools::ParseString( config, kKey_ReadyString, "BehaviorKnowledgeGraphQuestion" );

  std::string earConString;
  if ( JsonTools::GetValueOptional( config, kKey_EarConEnd, earConString ) )
  {
    _iVars.earConEnd = AudioMetaData::GameEvent::GenericEventFromString( earConString );
  }

  SubscribeToTags({{
    ExternalInterface::MessageEngineToGameTag::TouchButtonEvent,
    ExternalInterface::MessageEngineToGameTag::RobotFallingEvent, // do we need this?
  }});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const
{
  expectedKeys.insert( kKey_Duration );
  expectedKeys.insert( kKey_ReadyString );
  expectedKeys.insert( kKey_EarConEnd );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::InitBehavior()
{
  const BehaviorContainer& container = GetBEI().GetBehaviorContainer();
  container.FindBehaviorByIDAndDowncast<BehaviorTextToSpeechLoop>( BEHAVIOR_ID(KnowledgeGraphTTS),
                                                                   BEHAVIOR_CLASS(TextToSpeechLoop),
                                                                   _iVars.ttsBehavior );

  _readyTTSWrapper.Initialize( GetBEI().GetTextToSpeechCoordinator() );
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
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
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

  // start generating our ready text; if we fail, then we'll simply exit the behavior, and cry :(
  if ( _readyTTSWrapper.SetUtteranceText( _iVars.readyText, {} ) )
  {
    auto callback = [this]()
    {
      // after our getin animation, we can prompt the user to speak
      _dVars.state = EState::WaitingToStream;

      // Need to loop this forever and we'll just cancel it on our own after a timeout
      DelegateIfInControl( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphListening, 0 ) );
    };

    // open up streaming after we play our get-in to avoid motor noise
    DelegateIfInControl( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphGetIn ), callback );
  }
  else
  {
    PRINT_NAMED_WARNING( "BehaviorKnowledgeGraphQuestion", "Failed to generate Ready TTS (%s)", _iVars.readyText.c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::OnBehaviorDeactivated()
{
  // make sure we cancel the ready utterance if we bail during it playing
  _readyTTSWrapper.CancelUtterance();
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

    // at this point our get in animation is complete, so as soon as the audio is finished playing we can transition in
    if ( EState::WaitingToStream == _dVars.state )
    {
      // once we've finished speaking our ready text, we can start streaming
      // hopefully the ready text is already finished by the time we even get into this state
      if ( _readyTTSWrapper.IsFinished() )
      {
        BeginStreamingQuestion();
      }
      else if ( !_readyTTSWrapper.IsValid() )
      {
        PRINT_NAMED_WARNING( "BehaviorKnowledgeGraphQuestion", "Ready prompt TTS failed to play, not cool man" );

        // we need to bail, luckily TransitionToNoResponse() handles this exact transition for us
        CancelDelegates( false );
        TransitionToNoResponse();
      }
    }
    // if we're recording the user's question, we need to be listening for a response
    else if ( EState::Listening == _dVars.state )
    {
      // if we've gotten a response from knowledge graph, transition into the response state
      const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
      if(FLT_NEAR(_dVars.streamingBeginTime, 0.f) &&
         uic.IsCloudStreamOpen()){
        _dVars.streamingBeginTime = currentTime;
      }

      const bool timeIsUp = ( !FLT_NEAR( _dVars.streamingBeginTime, 0.f ) ) &&
                              ( currentTime >= ( _dVars.streamingBeginTime + _iVars.streamingDuration ) );
      if ( IsResponsePending() || timeIsUp )
      {
        CancelDelegates( false );
        OnStreamingComplete();
      }
    }
    else if ( EState::Responding == _dVars.state )
    {
      const bool isPickedUp = GetBEI().GetRobotInfo().IsPickedUp();

      // if we're playing our response, allow victor to be interrupted
      // require victor to be on the ground prior to being picked up, else ignore it if he's already in the air
      if ( !_dVars.wasPickedUp && isPickedUp )
      {
        OnResponseInterrupted();
      }

      _dVars.wasPickedUp = isPickedUp;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::BeginStreamingQuestion()
{
  PRINT_DEBUG( "Knowledge Graph streaming begun ..." );

  _dVars.state = EState::Listening;

  GetBehaviorComp<UserIntentComponent>().StartWakeWordlessStreaming( CloudMic::StreamType::KnowledgeGraph );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::OnStreamingComplete()
{
  // go into searching state until we can generate our response
  _dVars.state = EState::Searching;

  PlayEarconEnd();

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
    messageAnimation->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearchingGetIn ), true );
    messageAnimation->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearching ), true );

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
    // this shouldn't really happen, but handle it safely regardless
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
    auto callback = [this]()
    {
      // don't play the success anim if we've been interrupted
      if ( EState::Interrupted != _dVars.state )
      {
        // play our "woot woot" animation
        DelegateIfInControl( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSuccessReaction ) );
      }
    };

    DelegateIfInControl( _iVars.ttsBehavior.get(), callback );
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
      // TTS generation is done, so let's transition out of the searching animation and into the hearts all over the world!!!
      DelegateIfInControl( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearchingGetOutSuccess ),
                           &BehaviorKnowledgeGraphQuestion::TransitionToBeginResponse );
    }
    else
    {
      // we failed to generate the tts, but since we've already looped our searching anim, we just need to get out now
      CompoundActionSequential* messageAnimation = new CompoundActionSequential();
      messageAnimation->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearchingFail ), true );
      messageAnimation->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearchingFailGetOut ), true );

      DelegateIfInControl( messageAnimation );

      // ... annnnnd we're done
    }
  }
  else
  {
    DelegateIfInControl( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearching ),
                         &BehaviorKnowledgeGraphQuestion::TransitionToSearchingLoop );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToBeginResponse()
{
  _dVars.state = EState::Responding;

  DEV_ASSERT( !_dVars.responseString.empty(), "Responding to knowledge graph request but no response string exists" );

  _dVars.wasPickedUp = GetBEI().GetRobotInfo().IsPickedUp();

  // nothing to do but speak the response
  BeginResponseTTS();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToNoResponse()
{
  _dVars.state = EState::NoResponse;

  // our get-out from listening is simply a series of animations
  CompoundActionSequential* messageAnimation = new CompoundActionSequential();
  messageAnimation->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearchingGetIn ), true );
  messageAnimation->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearching ), true );
  messageAnimation->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearchingFail ), true );
  messageAnimation->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::KnowledgeGraphSearchingFailGetOut ), true );

  DelegateIfInControl( messageAnimation );

  // ... annnnnd we're done
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToNoConnection()
{
  PRINT_DEBUG( "Knowledge Graph not connected" );

  _dVars.state = EState::NoConnection;

  // currently no distinction between "no response", but there will/may be at some point
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::PlayEarconEnd()
{
  using namespace AudioMetaData::GameEvent;
  BehaviorExternalInterface& bei = GetBEI();

  if ( GenericEvent::Invalid != _iVars.earConEnd )
  {
    // Play earcon end audio
    bei.GetRobotAudioClient().PostEvent( _iVars.earConEnd, AudioMetaData::GameObjectType::Behavior );
  }
}

} // namespace Vector
} // namespace Anki
