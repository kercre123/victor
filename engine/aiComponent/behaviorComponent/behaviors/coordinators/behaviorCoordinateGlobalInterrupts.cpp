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
#include "engine/aiComponent/behaviorComponent/behaviors/behaviorHighLevelAI.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorTimerUtilityCoordinator.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "util/helpers/boundedWhile.h"

namespace Anki {
namespace Cozmo {

namespace{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::DynamicVariables::DynamicVariables()
{
}


///////////
/// BehaviorCoordinateGlobalInterrupts
///////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::BehaviorCoordinateGlobalInterrupts(const Json::Value& config)
: BehaviorDispatcherPassThrough(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::~BehaviorCoordinateGlobalInterrupts()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::InitPassThrough()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.wakeWordBehavior         = BC.FindBehaviorByID(BEHAVIOR_ID(TriggerWordDetected));

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(TimerUtilityCoordinator),
                                 BEHAVIOR_CLASS(TimerUtilityCoordinator),
                                 _iConfig.timerCoordBehavior);
  
  _iConfig.highLevelAIBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(HighLevelAI));
  _iConfig.sleepingBehavior    = BC.FindBehaviorByID(BEHAVIOR_ID(Sleeping));

  _iConfig.triggerWordPendingCond = BEIConditionFactory::CreateBEICondition(BEIConditionType::TriggerWordPending, GetDebugLabel());
  _iConfig.triggerWordPendingCond->Init(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::OnPassThroughActivated() 
{
  _iConfig.triggerWordPendingCond->SetActive(GetBEI(), true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::PassThroughUpdate()
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
  
  if( triggerWordPending ) {
    bool highLevelRunning = false;
    const IBehavior* behavior = this;
    const auto& bsm = GetBEI().GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<BehaviorSystemManager>();
    BOUNDED_WHILE( 100, (behavior != nullptr) && "Stack too deep to find sleeping" ) {
      behavior = bsm.GetBehaviorDelegatedTo( behavior );
      if( highLevelRunning && (behavior == _iConfig.sleepingBehavior.get()) ) {
        // High level AI is running the Sleeping behavior (probably through the Napping state).
        // Wake word serves as the wakeup for a napping robot, so disable the wake word behavior and let
        // high level AI resume. It will clear the pending trigger and resume in some other state. (The
        // wake up animation is the getout for napping)
        _iConfig.wakeWordBehavior->SetDontActivateThisTick(GetDebugLabel());
        
        break;
      }
      if( behavior == _iConfig.highLevelAIBehavior.get() ) {
        highLevelRunning = true;
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::OnPassThroughDeactivated()
{
  _iConfig.triggerWordPendingCond->SetActive(GetBEI(), false);
}


} // namespace Cozmo
} // namespace Anki
