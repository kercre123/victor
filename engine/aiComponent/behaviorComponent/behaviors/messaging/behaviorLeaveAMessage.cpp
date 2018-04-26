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

// #include "engine/aiComponent/behaviorComponent/behaviors/messaging/behaviorLeaveAMessage.h"
#include "behaviorLeaveAMessage.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/voiceMessageSystem.h"

#include "coretech/common/engine/utils/timer.h"

#include "util/logging/logging.h"

#include "clad/types/behaviorComponent/userIntent.h"


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLeaveAMessage::InstanceConfig::InstanceConfig()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLeaveAMessage::DynamicVariables::DynamicVariables() :
  startedRecordingTime( 0.0f )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLeaveAMessage::BehaviorLeaveAMessage( const Json::Value& config ) :
  ICozmoBehavior( config )
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::InitBehavior()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = false;
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

  // play an animation

  // who are we recording this message for?
  const UserIntent& intent = GetTriggeringUserIntent();
  DEV_ASSERT_MSG( ( intent.GetTag() == USER_INTENT( message_record ) ), "BehaviorLeaveAMessage",
                    "Expecting intent of type [message_record] but received type [%s]",
                    UserIntentTagToString( intent.GetTag() ) );

  const UserIntent_RecordMessage& messageIntent = intent.Get_message_record();
  _dVars.messageRecipient = messageIntent.given_name;

  PRINT_NAMED_DEBUG( "BehaviorLeaveAMessage", "Leaving a message for [%s]", _dVars.messageRecipient.c_str() );

  // start recording our message
  VoiceMessageSystem& voicemail = GetBEI().GetMicComponent().GetVoiceMessageSystem();
  if ( voicemail.CanRecordNewMessage() )
  {
    // note: have the behavior specify a duration within the .json config and pass said duration into
    //       VoiceMessageSystem::StartRecordingMessage()
    //       Also, look into setting a callback from VoiceMessageSystem
    voicemail.StartRecordingMessage( _dVars.messageRecipient );
    _dVars.startedRecordingTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::OnBehaviorDeactivated()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLeaveAMessage::BehaviorUpdate()
{
  if ( IsActivated() )
  {
    // note: temp until we read duration from .json
    const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ( currentTime >= ( _dVars.startedRecordingTime + 10.0f ) )
    {
      CancelSelf();
      PRINT_NAMED_DEBUG( "BehaviorLeaveAMessage", "Message finished recording" );
    }
  }
}

} // namespace Cozmo
} // namespace Anki
