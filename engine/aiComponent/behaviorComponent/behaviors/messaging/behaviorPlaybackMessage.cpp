/***********************************************************************************************************************
 *
 *  PlaybackMessage
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 4/025/2018
 *
 *  Description
 *  + Parent behavior for allowing the user to playback messages that have been previously recorded to Victor's local
 *    storage.
 *
 **********************************************************************************************************************/

// #include "engine/aiComponent/behaviorComponent/behaviors/messaging/behaviorPlaybackMessage.h"
#include "behaviorPlaybackMessage.h"
#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/voiceMessageSystem.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/logging/logging.h"

#include "clad/types/behaviorComponent/userIntent.h"


// todo notes:
// + need to properly localize tts strings
// + move the part where we say what messages we have (PlayNextRecipientTTS()) into it's own behavior as this will be used elsewhere
// + there are many common helper functions and things used between the messaging behavior, create a helpers class


#define PRINT_DEBUG( format, ... ) \
  PRINT_CH_DEBUG( "VoiceMessage", "BehaviorPlaybackMessage", format, ##__VA_ARGS__ )

#define PRINT_INFO( format, ... ) \
  PRINT_CH_INFO( "VoiceMessage", "BehaviorPlaybackMessage", format, ##__VA_ARGS__ )

namespace Anki {
namespace Vector {

namespace
{
  const char* kNameIdentifier             = "_name_";
  const char* kNumberIdentifier           = "_num_";

  const char* kTTSKey_AnnounceSingle      = "announceSingle";
  const char* kTTSKey_AnnouncePlural      = "announcePlural";
  const char* kTTSKey_ErrorNoRecipient    = "noRecipient";
  const char* kTTSKey_ErrorNoMessages     = "noMessagesForRecipient";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaybackMessage::InstanceConfig::InstanceConfig()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaybackMessage::DynamicVariables::DynamicVariables() :
  activeMessageId( kInvalidVoiceMessageId )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaybackMessage::BehaviorPlaybackMessage( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  _iVars.ttsAnnounceSingle    = JsonTools::ParseString( config, kTTSKey_AnnounceSingle, "BehaviorPlaybackMessage" );
  _iVars.ttsAnnouncePlural    = JsonTools::ParseString( config, kTTSKey_AnnouncePlural, "BehaviorPlaybackMessage" );
  _iVars.ttsErrorNoRecipient  = JsonTools::ParseString( config, kTTSKey_ErrorNoRecipient, "BehaviorPlaybackMessage" );
  _iVars.ttsErrorNoMessages   = JsonTools::ParseString( config, kTTSKey_ErrorNoMessages, "BehaviorPlaybackMessage" );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kTTSKey_AnnounceSingle );
  expectedKeys.insert( kTTSKey_AnnouncePlural );
  expectedKeys.insert( kTTSKey_ErrorNoRecipient );
  expectedKeys.insert( kTTSKey_ErrorNoMessages );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::InitBehavior()
{
  const BehaviorContainer& container = GetBEI().GetBehaviorContainer();
  container.FindBehaviorByIDAndDowncast<BehaviorTextToSpeechLoop>( BEHAVIOR_ID(MessagingPlaybackTTS),
                                                                   BEHAVIOR_CLASS(TextToSpeechLoop),
                                                                   _iVars.ttsBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  delegates.insert( _iVars.ttsBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlaybackMessage::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::OnBehaviorActivated()
{
  _dVars = DynamicVariables();

  // play an animation

  auto& uic = GetBehaviorComp<UserIntentComponent>();
  UserIntentPtr activeIntentPtr = uic.GetUserIntentIfActive( USER_INTENT( message_playback ) );

  DEV_ASSERT_MSG( activeIntentPtr != nullptr &&
                  ( activeIntentPtr->intent.GetTag() == USER_INTENT( message_playback ) ),
                  "BehaviorPlaybackMessage.OnBehaviorActivated.IncorrectIntent",
                  "Expecting intent of type [message_playback] but received type [%s]",
                  activeIntentPtr ? UserIntentTagToString( activeIntentPtr->intent.GetTag() ) : "NULL");

  // store the intended recipient to use either for playback or error response
  const UserIntent_PlaybackMessage& messageIntent = activeIntentPtr->intent.Get_message_playback();
  _dVars.messageRecipient = messageIntent.given_name;

  if ( !_dVars.messageRecipient.empty() )
  {
    // grab all of the messages for the user from the voicemail system
    // * grabbing newest to oldest since we playback from the end of the queue
    VoiceMessageSystem& voicemail = GetBEI().GetMicComponent().GetVoiceMessageSystem();
    _dVars.messages = voicemail.GetUnreadMessages( _dVars.messageRecipient, VoiceMessageSystem::SortType::NewestToOldest );

    // if we have messages, go through our message flow
    if ( !_dVars.messages.empty() )
    {
      TransitionToPlayingFirstMessage();
      return;
    }
  }

  TransitionToFailureResponse();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::OnBehaviorDeactivated()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::PlayTextToSpeech( const std::string& ttsString, BehaviorSimpleCallback callback )
{
  std::string textToSay = ttsString;

  // we need to replace all of our identifier strings with their actual values ...
  Util::StringReplace( textToSay, kNumberIdentifier, std::to_string( _dVars.messages.size() ) );

  // print this before we replace the name identifier for privacy reasons
  PRINT_DEBUG( "TTS: %s", textToSay.c_str() );

  Util::StringReplace( textToSay, kNameIdentifier, _dVars.messageRecipient );

  // delegate to our tts behavior
  _iVars.ttsBehavior->SetTextToSay( textToSay );
  if ( _iVars.ttsBehavior->WantsToBeActivated() )
  {
    DelegateIfInControl( _iVars.ttsBehavior.get(), callback );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::PlaybackNextMessage()
{
  DEV_ASSERT_MSG( !_dVars.messages.empty(),
                  "BehaviorPlaybackMessage.PlaybackNextMessage",
                  "Attempting to play next message but the list is empty!" );

  _dVars.activeMessageId = kInvalidVoiceMessageId;
  if ( !_dVars.messages.empty() )
  {
    // grab the next message in the list and pop it from the list
    _dVars.activeMessageId = _dVars.messages.back();
    _dVars.messages.pop_back();

    VoiceMessageSystem& voicemail = GetBEI().GetMicComponent().GetVoiceMessageSystem();
    voicemail.PlaybackRecordedMessage( _dVars.activeMessageId, [this]()
    {
      if ( IsActivated() && ( kInvalidVoiceMessageId != _dVars.activeMessageId ) )
      {
        OnMessagePlaybackComplete( _dVars.activeMessageId );
      }
    } );

    PRINT_DEBUG( "Playing message id %d", (int)_dVars.activeMessageId );

    // we assume any get-in animation has already been played, so we simply jump right into the looping anim
    DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::MessagingMessageLoop, 0 ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::OnMessagePlaybackComplete( VoiceMessageID id )
{
  PRINT_DEBUG( "Completed playing message id %d", (int)id );

  // delete the completed message from the mailbox
  VoiceMessageSystem& voicemail = GetBEI().GetMicComponent().GetVoiceMessageSystem();
  voicemail.DeleteMessage( id );

  _dVars.activeMessageId = kInvalidVoiceMessageId;

  // we need to cancel the current looping animation
  CancelDelegates();

  // play the deleted animation before we decide what to do next
  DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::MessagingMessageDeletedShort ), [this]()
  {
    // keep playing our messages if we have any
    if ( !_dVars.messages.empty() )
    {
      TransitionToPlayingNextMessage();
    }
    else
    {
      TransitionToFinishedMessages();
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::BehaviorUpdate()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::TransitionToPlayingFirstMessage()
{
  // play our announcement tts
  const bool isPlural = ( _dVars.messages.size() > 1 );
  std::string textToSay = ( isPlural ? _iVars.ttsAnnouncePlural : _iVars.ttsAnnounceSingle );
  PlayTextToSpeech( textToSay, [this]()
  {
    // after we've made our announcement, play some anims then get into the message delivery
    CompoundActionSequential* messageAnimation = new CompoundActionSequential();
    messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::MessagingMessageGetIn ), true );
    messageAnimation->AddAction( new TriggerAnimationAction( AnimationTrigger::MessagingMessageRewind ) );

    DelegateIfInControl( messageAnimation, &BehaviorPlaybackMessage::PlaybackNextMessage );
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::TransitionToPlayingNextMessage()
{
  // just need to play the get-in animation before we start the next message
  TriggerAnimationAction* animationAction = new TriggerAnimationAction( AnimationTrigger::MessagingMessageGetIn );
  DelegateIfInControl( animationAction, &BehaviorPlaybackMessage::PlaybackNextMessage );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::TransitionToFailureResponse()
{
  const VoiceMessageSystem& voicemail = GetBEI().GetMicComponent().GetVoiceMessageSystem();
  _dVars.allMessages = voicemail.GetUnreadMessages( VoiceMessageSystem::VoiceMessageSystem::SortType::NewestToOldest );

  // if we have no messages for anybody, play some error tts
  if ( _dVars.allMessages.empty() )
  {
    if ( _dVars.messageRecipient.empty() )
    {
      PlayTextToSpeech( _iVars.ttsErrorNoRecipient );
    }
    else
    {
      TransitionToNoMessagesResponse();
    }
  }
  else
  {
    // if we have messages for people, but not the user they asked for, simply rhyme off the names that we have
    PlayNextRecipientTTS();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::TransitionToNoMessagesResponse()
{
  DEV_ASSERT_MSG( _dVars.messages.empty(),
                  "BehaviorPlaybackMessage.TransitionToNoMessagesResponse",
                  "Transitioning to the no messages state but we actually do have messages!" );

  PlayTextToSpeech( _iVars.ttsErrorNoMessages );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::TransitionToFinishedMessages()
{
  // this will bail since we don't delegate
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::PlayNextRecipientTTS()
{
  if ( !_dVars.allMessages.empty() )
  {
    // this allows us to reuse the identifier replacement in PlayTextToSpeech(...)
    _dVars.messageRecipient = _dVars.allMessages.back().recipient;
    _dVars.messages = std::move( _dVars.allMessages.back().messages );
    _dVars.allMessages.pop_back();

    const bool isPlural = ( _dVars.messages.size() > 1 );
    std::string textToSay = ( isPlural ? _iVars.ttsAnnouncePlural : _iVars.ttsAnnounceSingle );
    PlayTextToSpeech( textToSay, [this]()
    {
      // keep recursing this until we have no more messages
      PlayNextRecipientTTS();
    });
  }
}

} // namespace Vector
} // namespace Anki

#undef PRINT_DEBUG
#undef PRINT_INFO

