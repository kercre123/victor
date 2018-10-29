/**
 * File: BehaviorPromptUserForVoiceCommand.h
 *
 * Author: Sam Russell
 * Created: 2018-04-30
 *
 * Description: This behavior prompts the user for a voice command, then puts Victor into "wake-wordless streaming".
 *              To keep the prompt behavior simple, resultant UserIntents should be handled by the delegating behavior,
 *              or elsewhere in the behaviorStack.
 * 
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPromptUserForVoiceCommand__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPromptUserForVoiceCommand__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/backpackLights/engineBackpackLightComponentTypes.h"
#include "clad/cloud/mic.h"

namespace Anki {
namespace Vector {

// Fwd Declarations
class BehaviorTextToSpeechLoop;

class BehaviorPromptUserForVoiceCommand : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPromptUserForVoiceCommand();

  // Expected to be used only if the JSON config did not provide a (re)prompt.
  // The main prompt must be set by JSON config or this method before the behavior wants to be activated.
  void SetPrompt(const std::string& text);
  void SetReprompt(const std::string& text);
  
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPromptUserForVoiceCommand(const Json::Value& config);  

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override final;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:

  enum class EState {
    TurnToFace,
    Prompting,
    Listening,
    Thinking,
    IntentReceived,
    Reprompt
  };

  enum class EIntentStatus : uint8_t
  {
    IntentHeard,
    IntentUnknown,
    NoIntentHeard
  };

  void TransitionToTurnToFace();
  void TransitionToPrompting();
  void TransitionToListening();
  void TransitionToThinking();
  void TransitionToIntentReceived();
  void TransitionToReprompt();

  // Accessory helpers
  void CheckForPendingIntents();

  const std::string& GetVocalPromptString() const;

  struct InstanceConfig {
    InstanceConfig();

    CloudMic::StreamType streamType;

    AudioMetaData::GameEvent::GenericEvent earConSuccess;
    AudioMetaData::GameEvent::GenericEvent earConFail;

    std::string ttsBehaviorID;
    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;

    std::string vocalPromptString;
    std::string vocalResponseToIntentString;
    std::string vocalResponseToBadIntentString;
    std::string vocalRepromptString;

    uint8_t maxNumReprompts;
    bool shouldTurnToFace;
    bool stopListeningOnIntents;
    bool backpackLights;
    bool playListeningGetIn;
    bool playListeningGetOut;
    bool wasPromptSetFromJson;
    bool wasRepromptSetFromJson;
  };

  struct DynamicVariables {
    DynamicVariables();

    EState                    state;
    EIntentStatus             intentStatus;
    uint8_t                   repromptCount;
    std::string               vocalPromptString; // only used if !_iConfig.wasPromptSetFromJson
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPromptUserForVoiceCommand__
