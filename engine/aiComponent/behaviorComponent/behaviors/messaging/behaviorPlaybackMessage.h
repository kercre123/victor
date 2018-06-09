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

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaybackMessage_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaybackMessage_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/mics/voiceMessageTypes.h"


namespace Anki {
namespace Cozmo {

class BehaviorTextToSpeechLoop;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorPlaybackMessage : public ICozmoBehavior
{
  friend class BehaviorFactory;
  BehaviorPlaybackMessage( const Json::Value& config );


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Overrides from ICozmoBehavior

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override;


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal Structs


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Overrides from ICozmoBehavior

  virtual void InitBehavior() override;
  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State Transitions

  void TransitionToPlayingFirstMessage();
  void TransitionToPlayingNextMessage();
  void TransitionToFailureResponse();
  void TransitionToNoMessagesResponse();
  void TransitionToFinishedMessages();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helpers ...

  // plays the next message in our internal list (starting at the end)
  void PlaybackNextMessage();
  void OnMessagePlaybackComplete( VoiceMessageID id );

  void PlayNextRecipientTTS();
  void PlayTextToSpeech( const std::string& ttsString, BehaviorSimpleCallback callback = {} );
  

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Instance Vars are members that last the lifetime of the behavior and generally do not change (config vars)

  struct InstanceConfig
  {
    InstanceConfig();

    std::string ttsAnnounceSingle;
    std::string ttsAnnouncePlural;
    std::string ttsErrorNoRecipient;
    std::string ttsErrorNoMessages;

    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;

  } _iVars;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Dynamic Vars are members that change over the lifetime of the behavior and are generally reset every activation

  struct DynamicVariables
  {
    DynamicVariables();

    std::string          messageRecipient;
    VoiceMessageList     messages;
    VoiceMessageUserList allMessages;
    VoiceMessageID       activeMessageId;

  } _dVars;

}; // class BehaviorPlaybackMessage

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaybackMessage_H__
