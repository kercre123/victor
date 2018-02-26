/**
* File: behaviorCoordinateGlobalInterrupts.cpp
*
* Author: Kevin M. Karol
* Created: 2/22/18
*
* Description: Behavior responsible for handling special case needs 
* that require coordination across behavior global interrupts
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/coordinators/behaviorCoordinateGlobalInterrupts.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimGetInLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorTimerUtilityCoordinator.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

namespace{
}

///////////
/// BehaviorCoordinateGlobalInterrupts
///////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::BehaviorCoordinateGlobalInterrupts(const Json::Value& config)
: ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::~BehaviorCoordinateGlobalInterrupts()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.globalInterruptsBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(GlobalInterruptions));
  _iConfig.wakeWordBehavior         = BC.FindBehaviorByID(BEHAVIOR_ID(TriggerWordDetected));

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(TimerUtilityCoordinator),
                                 BEHAVIOR_CLASS(TimerUtilityCoordinator),
                                 _iConfig.timerCoordBehavior);

  _iConfig.triggerWordPendingCond = BEIConditionFactory::CreateBEICondition(BEIConditionType::TriggerWordPending, GetDebugLabel());
  _iConfig.triggerWordPendingCond->Init(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.globalInterruptsBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCoordinateGlobalInterrupts::WantsToBeActivatedBehavior() const 
{
  // always wants to be activated 
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::OnBehaviorActivated() 
{
  _iConfig.triggerWordPendingCond->SetActive(GetBEI(), true);
  // for now strict priority always wants to run
  ANKI_VERIFY(_iConfig.globalInterruptsBehavior->WantsToBeActivated(),
              "BehaviorCoordinateGlobalInterrupts.OnBehaviorActivated.NoDelegationAvailable",
              "");
  DelegateIfInControl(_iConfig.globalInterruptsBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
  const bool triggerWordPending = _iConfig.triggerWordPendingCond->AreConditionsMet(GetBEI());
  const bool isTimerRinging     = _iConfig.timerCoordBehavior->IsTimerRinging();
  if(triggerWordPending && isTimerRinging){
    // Timer is ringing and will handle the pending trigger word instead of the wake word behavior
    _iConfig.wakeWordBehavior->SetDontActivateThisTick(GetDebugLabel());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::OnBehaviorDeactivated()
{
  _iConfig.triggerWordPendingCond->SetActive(GetBEI(), false);
}


} // namespace Cozmo
} // namespace Anki
