/**
 * File: BehaviorTextToSpeechLoop.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-05-17
 *
 * Description: Play a looping animation while reciting text to speech
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTextToSpeechLoop__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTextToSpeechLoop__

#include "clad/audio/audioSwitchTypes.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimGetInLoop.h"

namespace Anki {
namespace Cozmo {

// Fwd Declarations
enum class UtteranceState;

class BehaviorTextToSpeechLoop : public ICozmoBehavior
{
public:
  using AudioTtsProcessingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing;
  
  virtual ~BehaviorTextToSpeechLoop();

  virtual bool WantsToBeActivatedBehavior() const override final;

  void SetTextToSay(const std::string& textToSay,
                    const AudioTtsProcessingStyle style = AudioTtsProcessingStyle::Default_Processed);

  // allow the TTS to be interrupted
  void Interrupt();

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorTextToSpeechLoop(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override  final{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.behaviorAlwaysDelegates         = false;
  }

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override final;
  virtual void OnBehaviorActivated() override final;
  virtual void BehaviorUpdate() override final;
  virtual void OnBehaviorDeactivated() override final;

private:

  enum class State{
    IdleLoop,
    GetIn,
    SpeakingLoop,
    GetOut,
    EmergencyGetOut
  };

  struct InstanceConfig {
    InstanceConfig();
    AnimationTrigger idleTrigger;
    AnimationTrigger getInTrigger;
    AnimationTrigger loopTrigger;
    AnimationTrigger getOutTrigger;
    AnimationTrigger emergencyGetOutTrigger;
    std::string      devTestUtteranceString;
    bool             triggeredByAnim;
    bool             idleDuringTTSGeneration;
    uint8_t          tracksToLock;
  };

  // NOTE: Because state set on this behavior before it is run must persist into running,
  //       Dynamic variables are reset in OnBehaviorDeactivated instead of OnBehaviorActivated
  struct DynamicVariables {
    DynamicVariables();
    std::string     textToSay;
    State           state;
    uint8_t         utteranceID;
    UtteranceState  utteranceState;
    bool            hasSentPlayCommand;
    bool            cancelOnNextUpdate; 
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void TransitionToIdleLoop();
  void TransitionToGetIn();
  void TransitionToSpeakingLoop();
  void TransitionToGetOut();
  void TransitionToEmergencyGetOut();

  void PlayUtterance();
  void OnUtteranceUpdated(const UtteranceState& state);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTextToSpeechLoop__
