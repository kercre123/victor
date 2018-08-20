/**
 * File: behaviorOnboarding.h
 *
 * Author: ross
 * Created: 2018-06-01
 *
 * Description: The linear progressor for onboarding. It manages interruptions common to all stages,
 *              messaging, calls to IOnboardingStage's for progresson logic, and delegation requested
 *              by the stages.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboarding__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboarding__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/engineTimeStamp.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {

namespace Util{
  class IConsoleFunction;
}
  
namespace Vector {

class BehaviorOnboardingDetectHabitat;
class BehaviorOnboardingInterruptionHead;
class IOnboardingStage;
enum class OnboardingStages : uint8_t;
enum class OnboardingSteps  : uint8_t;
  
class BehaviorOnboarding : public ICozmoBehavior
{
public: 
  virtual ~BehaviorOnboarding() = default;
  
  // Sets the activation stage, but doesn't save to nv storage or anything. Incoming messages do that
  void SetOnboardingStage( OnboardingStages stage ) { _dVars.currentStage = stage; }

  const static std::string kOnboardingFolder;
  const static std::string kOnboardingFilename;
  const static std::string kOnboardingStageKey;
  
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboarding(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual void AlwaysHandleInScope(const GameToEngineEvent& event) override;
  virtual void AlwaysHandleInScope(const AppToEngineEvent& event) override;

private:
  
  // before dealing with any stages, make sure the eyes animation wakes up based on the current stage.
  // the initial wake up stage handles its own wake up animations
  enum class WakeUpState : uint8_t {
    Asleep=0,
    WakingUp,
    Complete,
  };
  
  enum class BehaviorState : uint8_t {
    StageNotStarted=0,
    StageRunning,
    Interrupted,
    InterruptedButComplete, // stage is done, but don't progress until the interruption finishes
    WaitingForTermination,
  };
  
  enum class WakeWordState : uint8_t {
    NotSet=0,
    TriggerDisabled,
    SpecialTriggerEnabledCloudDisabled,
    TriggerEnabled
  };
  
  using StagePtr = std::shared_ptr<IOnboardingStage>;
  
  // returns the current stage logic class, even if it is interrupted
  StagePtr& GetCurrentStage();
  
  void InitStages(bool resetExisting = true);
  
  // Checks for delegation and possibly delegates to interruptions. Returns true if an interruption is running
  bool CheckAndDelegateInterruptions();
  
  // Moves to the next IOnboardingStage
  void MoveToStage( const OnboardingStages& state );
  
  void SaveToDisk( const OnboardingStages& stage ) const;
  
  void UpdateBatteryInfo();
  void StartLowBatteryCountdown();
  
  // Interrupts any active stages with OnInterrupted(), caches info about interruptionID, and
  // messages the app for some interruptions so they can dim the screen or show a modal, etc.
  void Interrupt( ICozmoBehaviorPtr interruption, BehaviorID interruptionID );
  
  // Send app relevant messages for interruptions starting, ending, or changing
  void NotifyOfInterruptionChange( BehaviorID interruptionID );
  
  // App or devtool queues an event for continue/skip
  void RequestContinue( int step );
  void RequestSkip();
  void RequestRetryCharging();
  void RequestSkipRobotOnboarding();
  
  // Put this behavior in a state where it waits for the BSM to switch behavior stacks
  void TerminateOnboarding();
  
  void SetAllowedIntent(UserIntentTag tag);
  void SetAllowAnyIntent();
  
  void SendContinueResponse( bool acceptedContinue, int step );
  
  void SetRobotExpectingStep( int step );
  
  void SetWakeWordState( WakeWordState wakeWordState );
  
  bool CanInterruptionActivate( BehaviorID interruptionID ) const;
  
  int GetPhysicalInterruptionMsgType( BehaviorID interruptionID ) const;
  
  void OnDelegateComplete();
  
  void SendStageToApp( const OnboardingStages& stage ) const;

  // all events are queued so that their order is the same whether its an app event or dev tools event.
  // this holds the event data.
  struct PendingEvent {
    explicit PendingEvent(const EngineToGameEvent& event);
    explicit PendingEvent(const GameToEngineEvent& event);
    explicit PendingEvent(const AppToEngineEvent& event);
    PendingEvent(){} // only for Continue and Skip
    ~PendingEvent();
    enum {
      Continue,
      Skip,
      GameToEngine,
      EngineToGame,
      AppToEngine,
    } type;
    double time_s;
    union {
      GameToEngineEvent gameToEngineEvent; // GameToEngine
      EngineToGameEvent engineToGameEvent; // EngineToGame
      AppToEngineEvent  appToEngineEvent;  // AppToEngine
      int               continueNum;       // Continue
      // Skip has no params
    };
  };
  
  struct BatteryInfo {
    enum class ChargerState : uint8_t {
      Uninitialized=0,
      OnCharger,
      OffCharger,
    };
    bool lowBattery = false;
    ChargerState chargerState = ChargerState::Uninitialized;
    bool sentOutOfLowBattery = false;
    float timeChargingDone_s = -1.0f;
    float timeRemovedFromCharger = -1.0f;
  };

  struct InstanceConfig {
    InstanceConfig();
    
    BehaviorID wakeUpID;
    ICozmoBehaviorPtr wakeUpBehavior;
    
    std::vector<BehaviorID> interruptionIDs;
    std::vector<ICozmoBehaviorPtr> interruptions;
    std::shared_ptr<BehaviorOnboardingInterruptionHead> pickedUpBehavior;
    std::shared_ptr<BehaviorOnboardingInterruptionHead> onChargerBehavior;
    std::shared_ptr<BehaviorOnboardingDetectHabitat> detectHabitatBehavior;
    
    ICozmoBehaviorPtr normalReactToRobotOnBackBehavior;
    ICozmoBehaviorPtr specialReactToRobotOnBackBehavior;
    
    std::unordered_map<OnboardingStages,StagePtr> stages;
    
    std::list<Anki::Util::IConsoleFunction> consoleFuncs;
    
    std::string saveFolder;
  };
  
  struct DynamicVariables {
    DynamicVariables();
    
    WakeUpState wakeUpState;
    BehaviorState state;
    
    OnboardingStages currentStage;
    EngineTimeStamp_t timeOfLastStageChange;
    
    IBehavior* lastBehavior;
    UserIntentTag lastWhitelistedIntent;
    int lastExpectedStep;
    BehaviorID lastInterruption;
    
    WakeWordState wakeWordState;
    
    // pending requests from the app, devtools, or messages requested by the stage. While app and
    // engine messages are received in a known order, dev events are not, and so to make sure the dev
    // event works the same way as the app will eventually work, they are all cached here until being
    // passed to the class for stage logic. This recreates some of the functionality of AsycMessageGateComponent,
    // but allows for different messages for different stages. map key is time_s
    std::multimap<double, PendingEvent> pendingEvents;
    // event handlers for messages requested by the stage
    std::vector<Signal::SmartHandle> stageEventHandles;
    
    // gets set to true after the stage's behavior finishes and before it is next processed in BehaviorUpdate,
    // so that a stage's OnBehaviorDeactivated gets called
    bool currentStageBehaviorFinished;
    
    bool robotWokeUp; // woke up and finished driving off the charger
    bool robotHeardTrigger; // first trigger word was used
    
    // when true, allow DriveOffChargerStraight and prevent OnboardingPlacedOnCharger
    bool shouldDriveOffCharger;

    BatteryInfo batteryInfo;
    
    bool devConsoleStagePending;
    OnboardingStages devConsoleStage;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboarding__
