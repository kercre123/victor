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
// + response can be interrupted by "petting him", picking him up, or wakeword
// - GetBEI().GetRobotInfo().IsPickedUp()
// - ExternalInterface::MessageEngineToGameTag::TouchButtonEvent,
// - ExternalInterface::MessageEngineToGameTag::RobotFallingEvent,
// * use BehaviorPromptUserForVoiceCommand for the question/response

namespace Anki {
namespace Cozmo {

namespace
{
  const char* kKey_Duration                       = "streamingTimeout";
  const float kDefaultDuration                    = 7.0f;

  // animations ...
  const AnimationTrigger kAnim_StreamingGetIn     = AnimationTrigger::MessagingMessageGetIn;
  const AnimationTrigger kAnim_StreamingLoop      = AnimationTrigger::MessagingMessageLoop;
  const AnimationTrigger kAnim_StreamingGetOut    = AnimationTrigger::MessagingMessageGetOut;
  const AnimationTrigger kAnim_NoResponse         = AnimationTrigger::BlackJack_VictorLose;
  const AnimationTrigger kAnim_ResponseComplete   = AnimationTrigger::BlackJack_VictorWin;
  const AnimationTrigger kAnim_NoConnection       = AnimationTrigger::BlackJack_VictorLose;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::InstanceConfig::InstanceConfig() :
  streamingDuration( kDefaultDuration )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::DynamicVariables::DynamicVariables() :
  state( EState::Listening ),
  streamingBeginTime( 0.0f )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnowledgeGraphQuestion::BehaviorKnowledgeGraphQuestion( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  JsonTools::GetValueOptional( config, kKey_Duration, _iVars.streamingDuration );
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

  // open up streaming to record the user's question
  BeginStreamingQuestion();

  // Play our "I'm listening" animation to prompt the user to being askig their question
  CompoundActionSequential* messageAnimation = new CompoundActionSequential();
  messageAnimation->AddAction( new TriggerAnimationAction( kAnim_StreamingGetIn ), true );
  messageAnimation->AddAction( new TriggerAnimationAction( kAnim_StreamingLoop, 0 ) );

  DelegateIfInControl( messageAnimation );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::OnBehaviorDeactivated()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::BehaviorUpdate()
{
  if ( IsActivated() )
  {
    // if we're recording the user's question, we need to be listening for a response
    if ( EState::Listening == _dVars.state )
    {
      // if we've gotten a response from knowledge graph, transition into the response state
      if ( IsResponsePending() )
      {
        // need to consume the response immediately or the system gets cranky
        ConsumeResponse();

        // if we have a valid response, go ahead and speak the truth,
        // else, we need to deliver the bad news
        if ( !_dVars.responseString.empty() )
        {
          OnStreamingComplete( std::bind( &BehaviorKnowledgeGraphQuestion::TransitionToBeginResponse, this ) );
        }
        else
        {
          OnStreamingComplete( std::bind( &BehaviorKnowledgeGraphQuestion::TransitionToNoResponse, this ) );
        }
      }
      else
      {
        // if we haven't heard back from the cloud before our timeout, assume it ain't happening
        const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if ( currentTime > ( _dVars.streamingBeginTime + _iVars.streamingDuration ) )
        {
          OnStreamingComplete( std::bind( &BehaviorKnowledgeGraphQuestion::TransitionToNoResponse, this ) );
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::BeginStreamingQuestion()
{
  PRINT_DEBUG( "Knowledge Graph streaming begun ..." );

  GetBEI().GetMicComponent().StartWakeWordlessStreaming( CloudMic::StreamType::KnowledgeGraph );
  _dVars.streamingBeginTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::OnStreamingComplete( BehaviorSimpleCallback next )
{
  // go into responding state first, then figure out how to reponde
  _dVars.state = EState::Responding;

  // streaming has ended so we need to cancel our looping animation and play our getout
  CancelDelegates();
  DelegateIfInControl( new TriggerAnimationAction( kAnim_StreamingGetOut ), next );
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

    PRINT_DEBUG( "Knowledge Graph Question:\n %s", Util::HidePersonallyIdentifiableInfo( intentResponse.query_text.c_str() ) );
    PRINT_DEBUG( "Knowledge Graph Response:\n %s", Util::HidePersonallyIdentifiableInfo( intentResponse.answer.c_str() ) );
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
  // delegate to our tts behavior
  _iVars.ttsBehavior->SetTextToSay( _dVars.responseString );
  if ( _iVars.ttsBehavior->WantsToBeActivated() )
  {
    DelegateIfInControl( _iVars.ttsBehavior.get(), [this]()
    {
      // note: this could also be implemented in the get out of the TTS behavior, check with animation team
      DelegateIfInControl( new TriggerLiftSafeAnimationAction( kAnim_ResponseComplete ) );

      // ... annnnnd we're done
    });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToBeginResponse()
{
  // this should already be set, but doens't hurt to ensure
  _dVars.state = EState::Responding;

  DEV_ASSERT( !_dVars.responseString.empty(), "Responding to knowledge graph request but no response string exists" );
  PRINT_DEBUG( "Received Knowledge Graph response" );

  // nothing to do but speak the response
  BeginResponseTTS();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToNoResponse()
{
  PRINT_DEBUG( "No response recieved from Knowledge Graph" );

  _dVars.state = EState::NoResponse;

  DelegateIfInControl( new TriggerLiftSafeAnimationAction( kAnim_NoResponse ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnowledgeGraphQuestion::TransitionToNoConnection()
{
  PRINT_DEBUG( "Knowledge Graph not connected" );

  _dVars.state = EState::NoConnection;

  DelegateIfInControl( new TriggerLiftSafeAnimationAction( kAnim_NoConnection ) );
}

} // namespace Cozmo
} // namespace Anki

#undef PRINT_DEBUG

