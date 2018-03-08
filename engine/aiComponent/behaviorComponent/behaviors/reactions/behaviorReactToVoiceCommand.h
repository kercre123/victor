/**
* File: behaviorReactToVoiceCommand.h
*
* Author: Lee Crippen
* Created: 2/16/2017
*
* Description: Simple behavior to immediately respond to the voice command keyphrase, while waiting for further commands.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/bodyLightComponentTypes.h"
#include "engine/micDirectionTypes.h"


namespace Anki {
namespace Cozmo {

class BehaviorReactToMicDirection;
  
class BehaviorReactToVoiceCommand : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;

  friend class BehaviorContainer;
  friend class BehaviorFactory;
  BehaviorReactToVoiceCommand(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;
  
  // Empty override of AddListener because the strategy that controls this behavior is a listener
  // The strategy controls multiple different behaviors and listeners are necessary for the other behaviors
  // since they are generic PlayAnim behaviors (reactToVoiceCommand_Wakeup)
  virtual void AddListener(ISubtaskListener* listener) override {};


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  enum class EState : uint8_t
  {
    Positioning,
    Listening,
    Thinking,
    IntentReceived,
  };

  enum class EIntentStatus : uint8_t
  {
    IntentHeard,
    IntentUnknown,
    NoIntentHeard,
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override
  {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }

  // specific default values can be used to easily set all of our different
  // playtest options.  "Lee Happiness" refers to how happy/sad each of the
  // different settings make Lee feel ... more noise == Lee sad
  void LoadLeeHappinessValues( const Json::Value& config );
  
  virtual void InitBehavior() override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

  void SetReactionDirection();
  MicDirectionIndex GetReactionDirection() const;
  void SetUserIntentStatus();

  // state / transition functions
  void StartListening();
  void StopListening();

  void TransitionToThinking();
  void TransitionToIntentReceived();

  void OnStreamingBegin();
  void OnStreamingEnd();

  void OnVictorListeningBegin();
  void OnVictorListeningEnd();

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  struct InstanceConfig
  {
    InstanceConfig();

    bool earConBegin;
    bool earConEnd;
    bool turnOnTrigger;
    bool turnOnIntent;
    bool exitOnIntents;

    bool backpackLights;

    // response behavior to hearing the trigger word (or intent)
    std::string reactionBehaviorString;
    std::shared_ptr<BehaviorReactToMicDirection> reactionBehavior;

  } _instanceVars;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  struct DynamicVariables
  {
    DynamicVariables();

    EState                    state;
    MicDirectionIndex         reactionDirection;
    BackpackLightDataLocator  lightsHandle;
    float                     streamingBeginTime;
    EIntentStatus             intentStatus;

  } _dynamicVars;
  
}; // class BehaviorReactToVoiceCommand

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__
