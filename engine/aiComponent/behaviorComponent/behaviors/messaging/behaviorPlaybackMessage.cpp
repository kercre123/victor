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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/voiceMessageSystem.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/logging/logging.h"

#include "clad/types/behaviorComponent/userIntent.h"


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaybackMessage::InstanceConfig::InstanceConfig()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaybackMessage::DynamicVariables::DynamicVariables() :
  startedRecordingTime( 0.0f )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaybackMessage::BehaviorPlaybackMessage( const Json::Value& config ) :
  ICozmoBehavior( config )
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::InitBehavior()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = false;
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
  UserIntentPtr activeIntentPtr = uic.GetActiveUserIntent( USER_INTENT( message_playback ) );

  DEV_ASSERT_MSG( activeIntentPtr != nullptr &&
                  ( activeIntentPtr->GetTag() == USER_INTENT( message_playback ) ),
                  "BehaviorPlaybackMessage.OnBehaviorActivated.IncorrectIntent",
                  "Expecting intent of type [message_playback] but received type [%s]",
                  activeIntentPtr ? UserIntentTagToString( activeIntentPtr->GetTag() ) : "NULL");

  const UserIntent_RecordMessage& messageIntent = activeIntentPtr->Get_message_record();
  _dVars.messageRecipient = messageIntent.given_name;

  PRINT_NAMED_DEBUG( "BehaviorPlaybackMessage", "Playback message request for [%s]", _dVars.messageRecipient.c_str() );

  // note: need to actually do something with the name
  //       currently just trying to get the shell behavior committed
  VoiceMessageSystem& voicemail = GetBEI().GetMicComponent().GetVoiceMessageSystem();
  if ( voicemail.HasUnreadMessages() )
  {
    voicemail.PlaybackFirstUnreadMessage();
    _dVars.startedRecordingTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::OnBehaviorDeactivated()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaybackMessage::BehaviorUpdate()
{
  if ( IsActivated() )
  {
    // note: temp until we store message duration within the VoiceMessageSystem
    const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ( currentTime >= ( _dVars.startedRecordingTime + 10.0f ) )
    {
      CancelSelf();
      PRINT_NAMED_DEBUG( "BehaviorPlaybackMessage", "Message finished recording" );
    }
  }
}

} // namespace Cozmo
} // namespace Anki
