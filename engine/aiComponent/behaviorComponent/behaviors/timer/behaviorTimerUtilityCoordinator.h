/**
* File: behaviorTimerUtilityCoordinator.h
*
* Author: Kevin M. Karol
* Created: 2/7/18
*
* Description: Behavior which coordinates timer related behaviors including setting the timer
* antics that the timer is still running and stopping the timer when it's ringing
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Behaviors_BehaviorTimerUtilityCoordinator_H__
#define __Engine_Behaviors_BehaviorTimerUtilityCoordinator_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorProceduralClock.h"

namespace Anki {
namespace Vector {

// forward declarations
class BehaviorAdvanceClock;
class BehaviorAnimGetInLoop;
class BehaviorProceduralClock;
class TimerUtility;
class UserIntent;
// Specified in .cpp
class AnticTracker;

class BehaviorTimerUtilityCoordinator : public ICozmoBehavior
{
public:
  virtual ~BehaviorTimerUtilityCoordinator();

  bool IsTimerRinging();

  void SuppressAnticThisTick(unsigned long tickCount);

  #if ANKI_DEV_CHEATS
  void DevSetForceAntic() { _lParams.shouldForceAntic = true; };

  void DevAdvanceAnticBySeconds(int seconds);
  #endif
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorTimerUtilityCoordinator(const Json::Value& config);

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.behaviorAlwaysDelegates = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual bool WantsToBeActivatedBehavior() const override;

private:
  struct InstanceParams{
    std::string timerRingingBehaviorStr;
    std::shared_ptr<BehaviorProceduralClock> setTimerBehavior;
    std::shared_ptr<BehaviorProceduralClock> anticDisplayClock;
    std::shared_ptr<BehaviorProceduralClock> timerCheckTimeBehavior;
    ICozmoBehaviorPtr                        anticBaseBehavior;
    ICozmoBehaviorPtr                        timerRingingBehavior;
    ICozmoBehaviorPtr                        timerAlreadySetBehavior;
    ICozmoBehaviorPtr                        iCantDoThatBehavior;
    std::shared_ptr<BehaviorAdvanceClock>    cancelTimerBehavior;
    std::unique_ptr<AnticTracker>            anticTracker;
    int                                      minValidTimer_s;
    int                                      maxValidTimer_s;

    int                                      touchTimeToCancelTimer_ms;
  };

  struct LifetimeParams{
    LifetimeParams();
    bool shouldForceAntic;
    unsigned long tickToSuppressAnticFor;
    bool touchReleasedSinceStartedRinging;
    bool robotPlacedDownSinceStartedRinging;
    float timeRingingStarted_s;
  };

  InstanceParams _iParams;
  LifetimeParams _lParams;

  bool TimerShouldRing() const;
  TimerUtility& GetTimerUtility() const;
  
  void SetupTimerBehaviorFunctions();

  void TransitionToSetTimer();
  void TransitionToPlayAntic();
  void TransitionToShowTimeRemaining();
  void TransitionToRinging();
  void TransitionToTimerAlreadySet();
  void TransitionToNoTimerToCancel();
  void TransitionToCancelTimer();
  void TransitionToInvalidTimerRequest();

  BehaviorProceduralClock::GetDigitsFunction BuildTimerFunction(bool soonest) const;

  // Functions called by Update to check for transitions
  void CheckShouldCancelRinging();
  void CheckShouldSetTimer();
  void CheckShouldCancelTimer();
  void CheckShouldPlayAntic();
  void CheckShouldShowTimeRemaining();
};

} // namespace Vector
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorTimerUtilityCoordinator_H__
