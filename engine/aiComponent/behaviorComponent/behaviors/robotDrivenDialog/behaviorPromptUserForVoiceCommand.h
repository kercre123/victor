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
  // Prompt strings should be localized by caller.
  void SetPromptString(const std::string& text);
  void SetRepromptString(const std::string& text);

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
    IntentSilence,
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

  struct InstanceConfig {
    InstanceConfig();

    CloudMic::StreamType streamType;

    AudioMetaData::GameEvent::GenericEvent earConSuccess;
    AudioMetaData::GameEvent::GenericEvent earConFail;

    std::string ttsBehaviorID;
    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;

    // Configurable localization keys
    std::string vocalPromptKey;
    std::string vocalResponseToIntentKey;
    std::string vocalResponseToBadIntentKey;
    std::string vocalRepromptKey;

    uint8_t maxNumReprompts;
    bool shouldTurnToFace;
    bool stopListeningOnIntents;
    bool backpackLights;
    bool playListeningGetIn;
    bool playListeningGetOut;
  };

  struct DynamicVariables {
    DynamicVariables();

    EState                    state;
    EIntentStatus             intentStatus;
    uint8_t                   repromptCount;

    // Override vocalPromptString configured by json?
    bool                      useDynamicPromptString = false;
    std::string               dynamicPromptString;

    // Override vocalRepromptString configured by json?
    bool                      useDynamicRepromptString = false;
    std::string               dynamicRepromptString;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  //
  // Localized string helpers.
  // Return values may be empty if corresponding localization key has not been configured.
  // Dynamic values (if set) will override configured values.
  //
  std::string GetVocalPromptString() const;
  std::string GetVocalRepromptString() const;
  std::string GetVocalResponseToIntentString() const;
  std::string GetVocalResponseToBadIntentString() const;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPromptUserForVoiceCommand__
