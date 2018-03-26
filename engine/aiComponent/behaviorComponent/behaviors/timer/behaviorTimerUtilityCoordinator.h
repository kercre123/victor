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

namespace Anki {
namespace Cozmo {

// forward declarations
class BehaviorProceduralClock;
class BehaviorAnimGetInLoop;
class TimerUtility;
class UserIntent;
// Specified in .cpp
class AnticTracker;

class BehaviorTimerUtilityCoordinator : public ICozmoBehavior
{
public:
  virtual ~BehaviorTimerUtilityCoordinator();

  bool IsTimerRinging();

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
    std::shared_ptr<BehaviorProceduralClock> setTimerBehavior;
    std::shared_ptr<BehaviorProceduralClock> timerAnticBehavior;
    std::shared_ptr<BehaviorAnimGetInLoop>   timerRingingBehavior;
    ICozmoBehaviorPtr                        timerAlreadySetBehavior;
    ICozmoBehaviorPtr                        iCantDoThatBehavior;
    ICozmoBehaviorPtr                        cancelTimerBehavior;
    std::unique_ptr<AnticTracker>            anticTracker;
    int                                      minValidTimer_s;
    int                                      maxValidTimer_s;
  };

  struct LifetimeParams{
    LifetimeParams();
    bool shouldForceAntic;
    std::unique_ptr<UserIntent> setTimerIntent;
  };

  InstanceParams _iParams;
  LifetimeParams _lParams;

  bool TimerShouldRing() const;
  TimerUtility& GetTimerUtility() const;
  
  void SetupTimerBehaviorFunctions() const;

  void TransitionToSetTimer();
  void TransitionToPlayAntic();
  void TransitionToRinging();
  void TransitionToTimerAlreadySet();
  void TransitionToNoTimerToCancel();
  void TransitionToCancelTimer();
  void TransitionToInvalidTimerRequest();


  // Functions called by Update to check for transitions
  void CheckShouldCancelRinging();
  void CheckShouldSetTimer();
  void CheckShouldCancelTimer();
  void CheckShouldPlayAntic();
};

} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorTimerUtilityCoordinator_H__
