/**
 * File: BehaviorOnboarding1p0.h
 *
 * Author: ross
 * Created: 2018-09-04
 *
 * Description: Hail Mary Onboarding (1.0)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboarding1p0__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboarding1p0__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
  
namespace Util{
  class IConsoleFunction;
}
  
namespace Vector {
  
class BehaviorOnboardingLookAtPhone;
enum class OnboardingStages : uint8_t;

class BehaviorOnboarding1p0 : public ICozmoBehavior
{
public: 
  virtual ~BehaviorOnboarding1p0() = default;
  
  // for 1.0, just save whatever is passed
  void SetOnboardingStage( OnboardingStages stage ) { SaveToDisk(stage); }
  
  const static std::string kOnboardingFolder;
  const static std::string kOnboardingFilename;
  const static std::string kOnboardingStageKey;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboarding1p0(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual void HandleWhileActivated(const AppToEngineEvent& event) override;

private:
  
  enum class State : uint8_t {
    NotStarted = 0,
    LookAtPhone,
    ReceivedWakeUp, // waiting for LookAtPhone to finish
    WakingUp,
    DriveOffCharger,
    WaitForPutDown,
    WaitForTriggerWord,
    WaitForTriggerWordOnCharger,
    TriggerWord,
    PowerOff,
    
    GetOut,
    WaitingForTermination,
  };
  
  enum class WakeWordState : uint8_t {
    NotSet = 0,
    Disabled,
    Enabled,
  };
  
  struct BatteryInfo {
    bool needsToCharge;
    bool onCharger;
  };
  
  void TransitionToPhoneIcon();
  void TryTransitioningToReceivedWakeUp(); // might instead just send battery state info
  void TransitionToWakingUp();
  void TransitionToDrivingOffCharger();
  void TransitionToWaitingForVC(); // for both WaitForTriggerWord and WaitForTriggerWordOnCharger
  void TransitionToWaitingForPutDown(); // either when disturbed during wakeup or waiting for trigger word
  void TransitionToTriggerWord();
  void TransitionToPoweringOff();
  void TransitionToGetOut();
  
  void MarkOnboardingComplete();
  void SendWakeUpFinished() const;
  void SetStage( OnboardingStages stage, bool forceSkipStackReset = true );
  void TerminateOnboarding( bool timeout );
  void SaveToDisk( OnboardingStages stage ) const;
  
  void DisableWakeWord();
  void EnableWakeWord();
  
  void UpdateBatteryInfo( bool sendEvent = true );
  float GetChargerTime() const;
  bool IsBatteryCountdownDone() const;
  
  bool ShouldCheckPowerOff() const;
  
  void FixStimAtMax();
  void UnFixStim();

  struct InstanceConfig {
    InstanceConfig();
    
    std::list<Anki::Util::IConsoleFunction> consoleFuncs;
    std::string saveFolder;
    
    std::shared_ptr<BehaviorOnboardingLookAtPhone> behaviorLookAtPhone;
    ICozmoBehaviorPtr behaviorWaitForTriggerWord;
    ICozmoBehaviorPtr behaviorWaitForTriggerWordOnCharger;
    ICozmoBehaviorPtr behaviorTriggerWord;
    ICozmoBehaviorPtr behaviorPowerOff;
  };

  struct DynamicVariables {
    DynamicVariables();
    
    State state;
    WakeWordState wakeWordState;
    
    bool receivedWakeUp;
    bool attemptedLeaveCharger;
    bool markedComplete;
    
    float triggerWordStartTime_s;
    float treadsStateEndTime_s;
    
    BatteryInfo batteryInfo;
    
    bool isStimMaxed;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboarding1p0__
