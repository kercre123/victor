/**
 * File: BehaviorOnboardingCoordinator.h
 *
 * Author: Sam Russell
 * Created: 2018-10-26
 *
 * Description: Modular App driven onboarding.
 *
 *     NOTE: Onboarding phase behaviors should comply with the following expectations to maintain App Authoritative
 *     onboarding design:
 *
 *     1. The coordinator will maintain an idle state with no wakeword responses, no mandatory physical reactions, and
 *        no cube connections. Phase behaviors should handle these features internally to promote Sep of Concerns.
 *
 *     2. The phase behavior should expect to be interrupted by the coordinator in the event of App Disconnection.
 *        The coordinator will return to the LookAtPhone phase any time the app disconnects, and await instruction.
 * 
 *     3. If not specified otherwise in the onboarding json file, the power button will do nothing in a given onboarding
 *        phase. If the phase allows the power button to operate, it must properly handle being resumed by 
 *        re-delegation from the coordinator if the power button is released prior to powering down. 
 *
 *     4. The phase behavior should not maintain any persistent state. The App will be responsible for
 *        restoring onboarding to the beginning of the last known phase upon reconnection.
 *
 *     5. A timeout specific to the phase behavior should be defined in the coordinator json file. The
 *        coordinator will be responsible for all timeouts during onboarding to centralize settings and code
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingCoordinator__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingCoordinator__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {

namespace Util{
  class IConsoleFunction;
}

namespace Vector {

class BehaviorOnboarding1p0;
enum class OnboardingStages : uint8_t;
enum class OnboardingPhase : uint8_t;
enum class OnboardingPhaseState : uint8_t;

class BehaviorOnboardingCoordinator : public ICozmoBehavior
{
public:
  virtual ~BehaviorOnboardingCoordinator() = default;

  const static std::string kOnboardingFolder;
  const static std::string kOnboardingFilename;
  const static std::string kOnboardingStageKey;
  const static std::string kOnboardingTimeKey;

  // Onboarding is a little weird in that it may need to handle messages from the App even if it isn't
  // currently active to appropriately coordinate around Mute and CustomerCare screens which reset the
  // behavior stack on Entry/Exit. This function is called from the OnboardingMessageHandler BCComponent
  // whether or not this behavior is currently active so messages from the App are always handled.
  void HandleOnboardingMessageFromApp(const AppToEngineEvent& event);

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingCoordinator(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual bool WantsToBeActivatedBehavior() const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct BatteryInfo {
    float chargerTime_s;
    bool needsToCharge;
    bool onCharger;
  };

  void TransitionToPhase(const OnboardingPhase& phase,
                         bool sendSetPhaseResponseToApp = false,
                         bool resumingPhaseAfterInterruption = false);
  void TransitionToPoweringOff();

  void OnPhaseComplete(const OnboardingPhase& phase);

  // Evaluates active timeouts and returns TRUE if any are expired
  bool ShouldExitDueToTimeout();

  void DelegateTo1p0Onboarding();

  void FixStimAtMax();
  void UnFixStim();

  void RestartOnboarding();
  void TerminateOnboarding();
  void SaveToDisk(OnboardingStages phase) const;

  void UpdateBatteryInfo(bool sendEvent = true);
  void SendBatteryInfoToApp();
  float GetChargerTime() const;
  bool IsBatteryCountdownDone() const;

  void SendSetPhaseResponseToApp();
  void SendChargeInfoResponseToApp();
  void SendPhaseProgressResponseToApp();
  void Send1p0WakeUpResponseToApp(bool canWakeUp);

  struct OnboardingPhaseData{
    std::string       behaviorIDStr;
    float             timeout_s;
    ICozmoBehaviorPtr behavior;
    bool              allowPowerOff;
  };

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);

    std::list<Anki::Util::IConsoleFunction> consoleFuncs;
    std::unordered_map<OnboardingPhase, OnboardingPhaseData> OnboardingPhaseMap;
    std::string saveFolder;

    ICozmoBehaviorPtr behaviorPowerOff;

    float startTimeout_s;
    float completionTimeout_s;
    float appDisconnectTimeout_s;
    float idleTimeout_s;
    bool allowPowerOffFromIdle;
  };

  struct DynamicVariables {
    DynamicVariables();
    BatteryInfo batteryInfo;

    // App commanded phase info
    OnboardingPhase pendingPhase;
    OnboardingPhase lastSetPhase;
    ICozmoBehaviorPtr lastSetPhaseBehavior;
    OnboardingPhaseState lastSetPhaseState;

    OnboardingPhase currentPhase;

    float nextTimeSendBattInfo_s;

    float globalExitTime_s;
    float phaseExitTime_s;
    float appDisconnectExitTime_s;

    bool markCompleteAndExitOnNextUpdate;
    bool skipOnboardingOnNextUpdate;

    bool emulate1p0Onboarding;
    bool started1p0WakeUp;

    bool onboardingStarted;
    bool waitingForAppReconnect;
    bool waitingForTermination;
    bool terminatedNaturally;

    bool shouldCheckPowerOff;
    bool isStimMaxed;
    bool poweringOff;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingCoordinator__
