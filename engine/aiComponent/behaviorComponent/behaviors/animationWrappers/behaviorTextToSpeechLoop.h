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

#include <functional>

namespace Anki {
namespace Vector {

// Fwd Declarations
enum class UtteranceState;

class BehaviorTextToSpeechLoop : public ICozmoBehavior
{
public:
  using AudioTtsProcessingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing;
  
  virtual ~BehaviorTextToSpeechLoop();

  virtual bool WantsToBeActivatedBehavior() const override final;

  // callback passes in true if generation was completed successfully, false if it failed to generate
  using UtteranceReadyCallback = std::function<void(bool)>;
  void SetTextToSay(const std::string& textToSay,
                    const UtteranceReadyCallback readyCallback = {},
                    const AudioTtsProcessingStyle style = AudioTtsProcessingStyle::Default_Processed);
  
  void ClearTextToSay();
  
  // true when the utterance has been generated
  // note: can also pass a callback into SetTextToSay(...)
  bool IsUtteranceReady() const;

  // allow the TTS to be interrupted
  // immediate true means it will cancel now and play emergency get out anim
  // immediate false means it will finish the next loop animation and then exit with the normal get out anim
  void Interrupt( bool immediate );

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
    bool            cancelOnNextLoop;
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

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTextToSpeechLoop__
