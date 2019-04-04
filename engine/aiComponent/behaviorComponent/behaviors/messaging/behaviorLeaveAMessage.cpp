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

#include "behaviorLeaveAMessage.h"
#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/localeComponent.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/voiceMessageSystem.h"
#include "engine/components/visionComponent.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "clad/types/animationTrigger.h"

#include <cstdio>


// todo notes:
// + need to implement "stop on pickup/tap" feature
// + there are many common helper functions and things used between the messaging behavior, create a helpers class
// + user metadata eg. VisionComponent::IsNameTaken(...) should be moved outside of the vision system
// + record/playback can be interrupted by "petting him", picking him up, or wakeword?


#define PRINT_DEBUG( format, ... ) \
  PRINT_CH_DEBUG( "VoiceMessage", "BehaviorLeaveAMessage", format, ##__VA_ARGS__ )

#define PRINT_INFO( format, ... ) \
  PRINT_CH_INFO( "VoiceMessage", "BehaviorLeaveAMessage", format, ##__VA_ARGS__ )


namespace Anki {
namespace Vector {

namespace
{
  // Configurable localization keys
  const char* kKey_ttsNoRecipientKey      = "ttsNoRecipientKey";
  const char* kKey_ttsUnknownRecipientKey = "ttsUnknownRecipientKey";
  const char* kKey_ttsMailboxFullKey      = "ttsMailboxFullKey";

  const char* kKey_RequireKnownUser     = "requireKnownUser";
  const char* kKey_Duration             = "recordDuration";
  const float kDefaultDuration          = 10.0f;

  CONSOLE_VAR( bool, kRequireKnownUser, "VoiceMessage", true );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLeaveAMessage::InstanceConfig::InstanceConfig() :
  recordDuration( kDefaultDuration ),
  requireKnownUser( true )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLeaveAMessage::DynamicVariables::DynamicVariables() :
  messageId( kInvalidVoiceMessageId )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLeaveAMessage::BehaviorLeaveAMessage( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  _iVars.ttsNoRecipientKey      = JsonTools::ParseString( config, kKey_ttsNoRecipientKey, "BehaviorLeaveAMessage.NoRecipient" );
  _iVars.ttsUnknownRecipientKey = JsonTools::ParseString( config, kKey_ttsUnknownRecipientKey, "BehaviorLeaveAMessage.UnknownRecipient" );
  _iVars.ttsMailboxFullKey      = JsonTools::ParseString( config, kKey_ttsMailboxFullKey, "BehaviorLeaveAMessage.MailboxFull" );

  JsonTools::GetValueOptional( config, kKey_Duration, _iVars.recordDuration );
  JsonTools::GetValueOptional( config, kKey_RequireKnownUser, _iVars.requireKnownUser );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kKey_ttsNoRecipientKey );
  expectedKeys.insert( kKey_ttsUnknownRecipientKey );
  expectedKeys.insert( kKey_ttsMailboxFullKey );
  expectedKeys.insert( kKey_Duration );
  expectedKeys.insert( kKey_RequireKnownUser );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::InitBehavior()
{
  const BehaviorContainer& container = GetBEI().GetBehaviorContainer();
  container.FindBehaviorByIDAndDowncast<BehaviorTextToSpeechLoop>( BEHAVIOR_ID(MessagingRecordTTS),
                                                                   BEHAVIOR_CLASS(TextToSpeechLoop),
                                                                   _iVars.ttsBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  delegates.insert( _iVars.ttsBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLeaveAMessage::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::OnBehaviorActivated()
{
  _dVars = DynamicVariables();

  // who are we recording this message for?
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  UserIntentPtr activeIntentPtr = uic.GetUserIntentIfActive( USER_INTENT( message_record ) );

  DEV_ASSERT_MSG( activeIntentPtr != nullptr &&
                  ( activeIntentPtr->intent.GetTag() == USER_INTENT( message_record ) ),
                  "BehaviorLeaveAMessage.OnBehaviorActivated.IncorrectIntent",
                  "Expecting intent of type [message_record] but received type [%s]",
                  activeIntentPtr ? UserIntentTagToString( activeIntentPtr->intent.GetTag() ) : "NULL");

  // store the intended recipient to use either for recording or error response
  const UserIntent_RecordMessage& messageIntent = activeIntentPtr->intent.Get_message_record();
  _dVars.messageRecipient = messageIntent.given_name;

  RecordMessageResult result =
  {
    .id     = kInvalidVoiceMessageId,
    .error  = EVoiceMessageError::InvalidMessage
  };

  // we require a specified user to deliver the message to; this user is expected to be a known user that has been
  // enrolled to victor's knowledge base (via meet victor)
  if ( !_dVars.messageRecipient.empty() )
  {
    // must have a enrolled user as the intended recipient
    VisionComponent& vision = GetBEI().GetVisionComponent();
    if ( !DoesRequireKnownUser() || vision.IsNameTaken( _dVars.messageRecipient ) )
    {
      VoiceMessageSystem& voicemail = GetBEI().GetMicComponent().GetVoiceMessageSystem();

      // recording finished callback
      const auto onRecordingFinished = [this]()
      {
        if ( IsActivated() && ( kInvalidVoiceMessageId != _dVars.messageId ) )
        {
          OnMessagedRecordingComplete();
        }
      };

      // start recording our message
      const uint32_t duration = _iVars.recordDuration * 1000.0f;
      result = voicemail.StartRecordingMessage( _dVars.messageRecipient, duration, onRecordingFinished );

      if ( EVoiceMessageError::Success == result.error )
      {
        PRINT_DEBUG( "Leaving a message for (%s)", Util::HidePersonallyIdentifiableInfo( _dVars.messageRecipient.c_str() ) );
        _dVars.messageId = result.id;
      }
    }
  }

  // handle all error cases in case we add some in the future
  switch ( result.error )
  {
    case EVoiceMessageError::Success:
      TransitionToRecordingMessage();
      break;

    case EVoiceMessageError::MailboxFull:
      TransitionToMailboxFull();
      break;

    case EVoiceMessageError::InvalidMessage:
      TransitionToInvalidRecipient();
      break;

    case EVoiceMessageError::MessageAlreadyActive:
      DEV_ASSERT_MSG( false, "BehaviorLeaveAMessage.OnBehaviorActivated",
                     "VoiceMessageSystem::StartRecordingMessage(...) return error that we did not handle [%d]",
                     (int)result.error );
      CancelSelf();
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::OnBehaviorDeactivated()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::BehaviorUpdate()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLeaveAMessage::DoesRequireKnownUser() const
{
  return ( _iVars.requireKnownUser && kRequireKnownUser );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::PlayTextToSpeech( const std::string& ttsString, BehaviorSimpleCallback callback )
{
  PRINT_DEBUG( "TTS: %s", Anki::Util::RemovePII(ttsString).c_str());

  // delegate to our tts behavior
  _iVars.ttsBehavior->SetTextToSay(ttsString);
  if ( _iVars.ttsBehavior->WantsToBeActivated() )
  {
    DelegateIfInControl( _iVars.ttsBehavior.get(), callback );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::TransitionToRecordingMessage()
{
  DEV_ASSERT_MSG( _dVars.messageId != kInvalidVoiceMessageId,
                  "BehaviorLeaveAMessage.TransitionToRecordingMessage",
                  "Transitioning to recording message but message id is invalid" );

  // play our recording animation here ...
  CompoundActionSequential* messageAnimation = new CompoundActionSequential();
  messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::MessagingMessageGetIn ), true );
  messageAnimation->AddAction( new ReselectingLoopAnimationAction( AnimationTrigger::MessagingMessageLoop ) );

  DelegateIfInControl( messageAnimation );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::OnMessagedRecordingComplete()
{
  PRINT_DEBUG( "Completed recording message id %d", (int)_dVars.messageId );

  _dVars.messageId = kInvalidVoiceMessageId;

  // cancel our looping message animation
  CancelDelegates();

  CompoundActionSequential* messageAnimation = new CompoundActionSequential();
  messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::MessagingMessageGetOut ), true );
  messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::MessagingMessageRecordReaction ) );

  DelegateIfInControl( messageAnimation );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::TransitionToInvalidRecipient()
{
  const auto & localeComponent = GetBEI().GetRobotInfo().GetLocaleComponent();

  if ( _dVars.messageRecipient.empty() )
  {
    // inform the user we didn't hear the name of the recipient
    const std::string & text = localeComponent.GetString(_iVars.ttsNoRecipientKey);
    PlayTextToSpeech(text);
  }
  else
  {
    // we don't know the recipient
    const std::string & text = localeComponent.GetString(_iVars.ttsUnknownRecipientKey, _dVars.messageRecipient);
    PlayTextToSpeech(text);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::TransitionToMailboxFull()
{
  // inform the user the mailbox is full
  const auto & localeComponent = GetBEI().GetRobotInfo().GetLocaleComponent();
  const auto & text = localeComponent.GetString(_iVars.ttsMailboxFullKey);
  PlayTextToSpeech(text);
}

} // namespace Vector
} // namespace Anki

#undef PRINT_DEBUG
#undef PRINT_INFO
