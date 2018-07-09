/**
* File: behaviorReactToVoiceCommand.h
*
* Author: Jarrod Hatfield
* Created: 2/16/2018
*
* Description: Simple behavior to immediately respond to the voice command keyphrase, while waiting for further commands.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__

#include "clad/audio/audioEventTypes.h"
#include "coretech/common/engine/utils/recentOccurrenceTracker.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/backpackLights/backpackLightComponentTypes.h"
#include "engine/components/mics/micDirectionTypes.h"


namespace Anki {
namespace Cozmo {

class BehaviorReactToMicDirection;
enum class AnimationTrigger : int32_t;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorReactToVoiceCommand : public ICozmoBehavior
{
  friend class BehaviorFactory;
  BehaviorReactToVoiceCommand(const Json::Value& config);


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override;
  
  // Empty override of AddListener because the strategy that controls this behavior is a listener
  // The strategy controls multiple different behaviors and listeners are necessary for the other behaviors
  // since they are generic PlayAnim behaviors (reactToVoiceCommand_Wakeup)
  virtual void AddListener(ISubtaskListener* listener) override {};

  // Allow other behaviors to specify a timestamp (generally the current timestamp)
  // on which the turn on trigger/intent is disabled
  void DisableTurnForTimestamp(TimeStamp_t timestampToDisableFor){
    _dVars.timestampToDisableTurnFor = timestampToDisableFor;
  }

protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  enum class EState : uint8_t
  {
    GetIn,
    Listening,
    Thinking,
    IntentReceived,
  };

  enum class EIntentStatus : uint8_t
  {
    IntentHeard,
    IntentUnknown,
    NoIntentHeard,
    Error
  };


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual void InitBehavior() override;
  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override;

  virtual void AlwaysHandleInScope( const RobotToEngineEvent& event ) override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

  // reaction direction functions ...

  // cache the direction we want to react to
  void ComputeReactionDirection();
  // get the direction we want to react to
  MicDirectionIndex GetReactionDirection() const;
  // get the "best recent" direction from the mic history
  MicDirectionIndex GetDirectionFromMicHistory() const;
  
  void UpdateUserIntentStatus();

  // state / transition functions
  void StartListening();
  void StopListening();

  void TransitionToThinking();
  void TransitionToIntentReceived();

  // coincide with the beginning of the stream being opened
  void OnStreamingBegin();

  // this is the state when victor is "listening" for the users intent
  // and should therefore cue the user to speak
  void OnVictorListeningBegin();
  void OnVictorListeningEnd();


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  struct InstanceConfig
  {
    InstanceConfig();

    // earcon is an audible cue to tell the user victor is listening
    AudioMetaData::GameEvent::GenericEvent earConBegin;
    AudioMetaData::GameEvent::GenericEvent earConSuccess;
    AudioMetaData::GameEvent::GenericEvent earConFail;
    
    AnimationTrigger animListeningGetIn;

    bool backpackLights;
    
    bool exitAfterGetIn;

    // response behavior to hearing the trigger word (or intent)
    std::string reactionBehaviorString;
    std::shared_ptr<BehaviorReactToMicDirection> reactionBehavior;

    // behaviors to handle specific failure cases
    ICozmoBehaviorPtr unmatchedIntentBehavior;
    ICozmoBehaviorPtr noCloudBehavior;
    ICozmoBehaviorPtr noWifiBehavior;

    // tracking for when to trigger failure behaviors
    float _errorTrackingWindow_s = 0.0f;
    int _numErrorsToTriggerAnim = 0;
    RecentOccurrenceTracker cloudErrorTracker;
    // when this handle's conditions are met, we animate to show the user there was a failure (and possibly
    // trigger an attention transfer)
    RecentOccurrenceTracker::Handle cloudErrorHandle;

  } _iVars;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  struct DynamicVariables
  {
    DynamicVariables();

    EState                    state;
    MicDirectionIndex         reactionDirection;
    BackpackLightDataLocator  lightsHandle;
    float                     streamingBeginTime;
    EIntentStatus             intentStatus;
    TimeStamp_t               timestampToDisableTurnFor;
  } _dVars;

  // these are dynamic vars that live beyond the activation scope ...
  MicDirectionIndex         _triggerDirection;

  bool IsTurnEnabled() const;

}; // class BehaviorReactToVoiceCommand

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__
