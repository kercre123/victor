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

// #include "engine/aiComponent/behaviorComponent/behaviors/knowledgeGraph/behaviorAskAQuestion.h"
#include "behaviorAskAQuestion.h"
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
#include "util/logging/logging.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/behaviorComponent/userIntent.h"

#define PRINT_DEBUG( format, ... ) \
  PRINT_CH_DEBUG( "KnowledgeGraph", "BehaviorAskAQuestion", format, ##__VA_ARGS__ )

#define PRINT_INFO( format, ... ) \
  PRINT_CH_INFO( "KnowledgeGraph", "BehaviorAskAQuestion", format, ##__VA_ARGS__ )

// notes:
// + need to check if Houndify connection is valid and respond accordingly if it is not
// + implement BeginStreamingQuestion()
// + implement IsResponsePending()
// + implement GetResponse()
// + response can be interrupted by "petting him", picking him up, or wakeword

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
BehaviorAskAQuestion::InstanceConfig::InstanceConfig() :
  streamingDuration( kDefaultDuration )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAskAQuestion::DynamicVariables::DynamicVariables() :
  state( EState::Listening ),
  streamingBeginTime( 0.0f )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAskAQuestion::BehaviorAskAQuestion( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  JsonTools::GetValueOptional( config, kKey_Duration, _iVars.streamingDuration );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const
{
  expectedKeys.insert( kKey_Duration );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::InitBehavior()
{
  const BehaviorContainer& container = GetBEI().GetBehaviorContainer();
  container.FindBehaviorByIDAndDowncast<BehaviorTextToSpeechLoop>( BEHAVIOR_ID(KnowledgeGraphTTS),
                                                                   BEHAVIOR_CLASS(TextToSpeechLoop),
                                                                   _iVars.ttsBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  delegates.insert( _iVars.ttsBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = false;
  modifiers.behaviorAlwaysDelegates               = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAskAQuestion::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::OnBehaviorActivated()
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
void BehaviorAskAQuestion::OnBehaviorDeactivated()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::BehaviorUpdate()
{
  if ( IsActivated() )
  {
    // if we're recording the user's question, we need to be listening for a response
    if ( EState::Listening == _dVars.state )
    {
      // if we've gotten a response from knowledge graph, transition into the response state
      if ( IsResponsePending() )
      {
        OnStreamingComplete( std::bind( &BehaviorAskAQuestion::TransitionToBeginResponse, this ) );
      }
      else
      {
        // if we haven't heard back from the cloud before our timeout, assume it ain't happening
        const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if ( currentTime > ( _dVars.streamingBeginTime + _iVars.streamingDuration ) )
        {
          OnStreamingComplete( std::bind( &BehaviorAskAQuestion::TransitionToNoResponse, this ) );
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::BeginStreamingQuestion()
{
 // implement me!
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::OnStreamingComplete( BehaviorSimpleCallback next )
{
  // streaming has ended so we need to cancel our looping animation and play our getout
  CancelDelegates();
  DelegateIfInControl( new TriggerAnimationAction( kAnim_StreamingGetOut ), next );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAskAQuestion::IsResponsePending() const
{
  // implement me!

  const std::string& responseString = GetResponse();
  return !responseString.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& BehaviorAskAQuestion::GetResponse() const
{
  // implement me!

  static const std::string kResponse = "Bentley is a dog";
  return kResponse;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::BeginResponseTTS()
{
  DEV_ASSERT( !_dVars.responseString.empty(), "Responding to knowledge graph request but no response string exists" );

  PRINT_DEBUG( "Response: %s", _dVars.responseString.c_str() );

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
void BehaviorAskAQuestion::TransitionToBeginResponse()
{
  _dVars.state = EState::Responding;

  // save our response string so we can TTS it to the user
  _dVars.responseString = GetResponse();

  // if the response string is empty, it means knowledge graph didn't know what's up, so respond accordingly
  if ( !_dVars.responseString.empty() )
  {
    BeginResponseTTS();
  }
  else
  {
    TransitionToNoResponse();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::TransitionToNoResponse()
{
  _dVars.state = EState::NoResponse;

  DelegateIfInControl( new TriggerLiftSafeAnimationAction( kAnim_NoResponse ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAskAQuestion::TransitionToNoConnection()
{
  _dVars.state = EState::NoConnection;

  DelegateIfInControl( new TriggerLiftSafeAnimationAction( kAnim_NoConnection ) );
}

} // namespace Cozmo
} // namespace Anki

#undef PRINT_DEBUG

