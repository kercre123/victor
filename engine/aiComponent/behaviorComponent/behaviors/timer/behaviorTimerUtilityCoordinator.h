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
#include "clad/types/behaviorComponent/userIntent.h"

namespace Anki {
namespace Cozmo {

// forward declarations
class BehaviorProceduralClock;
class BehaviorAnimGetInLoop;
class TimerUtility;
// Specified in .cpp
class AnticTracker;

class BehaviorTimerUtilityCoordinator : public ICozmoBehavior
{
public:
  virtual ~BehaviorTimerUtilityCoordinator();

  bool IsTimerRinging();

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorTimerUtilityCoordinator(const Json::Value& config);

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.behaviorAlwaysDelegates = true;
  }

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual bool WantsToBeActivatedBehavior() const override;

private:
  struct InstanceParams{
    std::shared_ptr<BehaviorProceduralClock> setTimerBehavior;
    std::shared_ptr<BehaviorProceduralClock> timerAnticBehavior;
    std::shared_ptr<BehaviorAnimGetInLoop> timerRingingBehavior;
    std::unique_ptr<AnticTracker> anticTracker;
  };

  struct LifetimeParams{
    mutable bool timerSet = false;
    std::unique_ptr<UserIntent> setTimerIntent = std::make_unique<UserIntent>();
  };

  InstanceParams _iParams;
  LifetimeParams _lParams;

  bool TimerShouldRing() const;
  const TimerUtility& GetTimerUtility() const;
  
  void SetupTimerBehaviorFunctions() const;

  void TransitionToSetTimer();
  void TransitionToPlayAntic();
  void TransitionToRinging();

};

} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorTimerUtilityCoordinator_H__
