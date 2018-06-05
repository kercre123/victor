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
#include "engine/components/bodyLightComponentTypes.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

// Fwd Declarations
class BehaviorTextToSpeechLoop;

class BehaviorPromptUserForVoiceCommand : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPromptUserForVoiceCommand();

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

  void OnStreamingBegin();
  void OnStreamingEnd();

  void OnVictorListeningBegin();
  void OnVictorListeningEnd();


  // Accessory helpers
  void CheckForPendingIntents();
  void TurnOffBackpackLights();
  IBehavior* SelectPromptBehavior(std::string& speechString, ICozmoBehaviorPtr& altPromptBehavior);

  struct InstanceConfig {
    InstanceConfig();

    // earcon is an audible cue to tell the user victor is listening
    AudioMetaData::GameEvent::GenericEvent earConBegin;
    AudioMetaData::GameEvent::GenericEvent earConSuccess;
    AudioMetaData::GameEvent::GenericEvent earConFail;

    std::string ttsBehaviorID;
    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;

    AnimationTrigger listenGetInOverrideTrigger;
    AnimationTrigger listenAnimOverrideTrigger;
    AnimationTrigger listenGetOutOverrideTrigger;

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

  };

  struct DynamicVariables {
    DynamicVariables();

    EState                    state;
    BackpackLightDataLocator  lightsHandle;
    float                     streamingBeginTime;
    EIntentStatus             intentStatus;
    bool                      isListening;
    uint8_t                   repromptCount;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPromptUserForVoiceCommand__
