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

#ifndef __Cozmo_Basestation_Behaviors_BehaviorLeaveAMessage_H__
#define __Cozmo_Basestation_Behaviors_BehaviorLeaveAMessage_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/mics/voiceMessageTypes.h"


namespace Anki {
namespace Vector {

class BehaviorTextToSpeechLoop;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorLeaveAMessage : public ICozmoBehavior
{
  friend class BehaviorFactory;
  BehaviorLeaveAMessage( const Json::Value& config );


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

  void TransitionToRecordingMessage();
  void TransitionToFailureResponse();
  void TransitionToInvalidRecipient();
  void TransitionToMailboxFull();

  void OnMessagedRecordingComplete();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helpers

  bool DoesRequireKnownUser() const;
  void PlayTextToSpeech( const std::string& ttsString, BehaviorSimpleCallback callback = {} );

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Instance Vars are members that last the lifetime of the behavior and generally do not change (config vars)

  struct InstanceConfig
  {
    InstanceConfig();

    float         recordDuration;
    bool          requireKnownUser;

    std::string   ttsErrorNoRecipient;
    std::string   ttsErrorUnknownRecipient;
    std::string   ttsErrorMailboxFull;

    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;

  } _iVars;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Dynamic Vars are members that change over the lifetime of the behavior and are generally reset every activation

  struct DynamicVariables
  {
    DynamicVariables();

    std::string          messageRecipient;
    VoiceMessageID       messageId;

  } _dVars;

}; // class BehaviorLeaveAMessage

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorLeaveAMessage_H__
