/**
 * File: behaviorPopAWheelie.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2016-06-15
 *
 * Description: Behavior that allows cozmo to use a block to throw himself on his back
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorPopAWheelie.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/robot.h"
#include "coretech/vision/engine/observableObject.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {

namespace{
CONSOLE_VAR(s32, kBPW_MaxRetries,         "Behavior.PopAWheelie", 1);

} // end namespace


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPopAWheelie::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPopAWheelie::DynamicVariables::DynamicVariables()
{
  numPopAWheelieActionRetries = 0;
  hasDisabledcliff = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPopAWheelie::BehaviorPopAWheelie(const Json::Value& config)
: ICozmoBehavior(config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPopAWheelie::WantsToBeActivatedBehavior() const
{
  UpdateTargetBlock();
  
  return _dVars.targetBlock.IsSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::OnBehaviorActivated()
{
  if(!ShouldStreamline() && _dVars.lastBlockReactedTo != _dVars.targetBlock){
    TransitionToReactingToBlock();
  }else{
    TransitionToPerformingAction();
  }
  
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::OnBehaviorDeactivated()
{
  ResetBehavior();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::UpdateTargetBlock() const
{
  _dVars.targetBlock = GetAIComp<ObjectInteractionInfoCache>().
       GetBestObjectForIntention(ObjectInteractionIntention::PopAWheelieOnObject);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToReactingToBlock()
{
  DEBUG_SET_STATE(ReactingToBlock);

  // Turn towards the object and then react to it before performing the pop a wheelie action
  DelegateIfInControl(new CompoundActionSequential({
      new TurnTowardsObjectAction(_dVars.targetBlock),
      new TriggerLiftSafeAnimationAction(AnimationTrigger::PopAWheelieInitial),
    }),
    [this](const ActionResult& res){
      if(res == ActionResult::SUCCESS)
      {
        _dVars.lastBlockReactedTo = _dVars.targetBlock;
      }
      TransitionToPerformingAction();
    });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToPerformingAction()
{
  DEBUG_SET_STATE(PerformingAction);
  TransitionToPerformingAction(false);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToPerformingAction(bool isRetry)
{
  if( ! _dVars.targetBlock.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorPopAWheelie.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetDebugLabel().c_str());
    return;
  }
  
  if(isRetry) {
    ++_dVars.numPopAWheelieActionRetries;
    PRINT_NAMED_INFO("BehaviorPopAWheelie.TransitionToPerformingAction.Retrying",
                     "Retry %d of %d", _dVars.numPopAWheelieActionRetries, kBPW_MaxRetries);
  } else {
    _dVars.numPopAWheelieActionRetries = 0;
  }
  
  // Only turn towards face if this is _not_ a retry
  const Radians maxTurnToFaceAngle( (isRetry || ShouldStreamline() ? 0 : DEG_TO_RAD(90)) );

  DriveToPopAWheelieAction* goPopAWheelie = new DriveToPopAWheelieAction(_dVars.targetBlock,
                                                                         false,
                                                                         0,
                                                                         maxTurnToFaceAngle);
  goPopAWheelie->SetSayNameAnimationTrigger(AnimationTrigger::PopAWheeliePreActionNamedFace);
  goPopAWheelie->SetNoNameAnimationTrigger(AnimationTrigger::PopAWheeliePreActionUnnamedFace);
  
  // once we get to the predock pose, before docking, disable the cliff sensor and associated reactions so
  // that we play the correct animation instead of getting interrupted)  
  auto disableCliff = [this](Robot& robot) {
    // tell the robot not to stop the current action / animation if the cliff sensor fires
    _dVars.hasDisabledcliff = true;
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));
  };
  goPopAWheelie->SetPreDockCallback(disableCliff);


  DelegateIfInControl(goPopAWheelie,
              [&, this](const ExternalInterface::RobotCompletedAction& msg) {                
                switch(IActionRunner::GetActionResultCategory(msg.result))
                {
                  case ActionResultCategory::SUCCESS:
                  {
                    _dVars.lastBlockReactedTo.UnSet();
                    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::SuccessfulWheelie));
                    BehaviorObjectiveAchieved(BehaviorObjective::PoppedWheelie);
                    break;
                  }
                  case ActionResultCategory::RETRY:
                  {
                    if(_dVars.numPopAWheelieActionRetries < kBPW_MaxRetries)
                    {
                      SetupRetryAction(msg);
                      break;
                    }
                    
                    // else: too many retries, fall through to failure abort
                  }
                  case ActionResultCategory::ABORT:
                  {
                    // assume that failure is because we didn't visually verify (although it could be due to an
                    // error)
                    PRINT_NAMED_INFO("BehaviorPopAWheelie.FailedAbort",
                                     "Failed to pop with %s, searching for block",
                                     EnumToString(msg.result));
                    
                    // mark the block as inaccessible
                    const ObservableObject* failedObject = failedObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetBlock);
                    if(failedObject){
                      GetAIComp<AIWhiteboard>().SetFailedToUse(*failedObject, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
                    }
                    break;
                  }
                  default:
                  {
                    // other failure, just end
                    PRINT_NAMED_INFO("BehaviorPopAWheelie.FailedPopAction",
                                     "action failed with %s, behavior ending",
                                     EnumToString(msg.result));
                  }
                } // switch(msg.result)

              });
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::SetupRetryAction(const ExternalInterface::RobotCompletedAction& msg)
{
  //Ensure that the closest block is selected
  UpdateTargetBlock();
  
  // Pick which retry animation, if any, to play. Then try performing the action again,
  // with "isRetry" set to true.
  
  IActionRunner* animAction = nullptr;
  switch(msg.result)
  {
    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    {
      animAction = new TriggerLiftSafeAnimationAction(AnimationTrigger::PopAWheelieRealign);
      break;
    }
      
    default: {
      animAction = new TriggerLiftSafeAnimationAction(AnimationTrigger::PopAWheelieRetry);
      break;
    }
  }
  
  if( nullptr != animAction ) {
    DelegateIfInControl(animAction, [this]() {
      this->TransitionToPerformingAction(true);
    });
  }
  else {
    TransitionToPerformingAction(true);
  }
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::ResetBehavior()
{
  _dVars.targetBlock.UnSet();

  if( _dVars.hasDisabledcliff ) {
    _dVars.hasDisabledcliff = false;
    auto& robotInfo = GetBEI().GetRobotInfo();
    // NOTE: assumes that we want the cliff to be re-enabled when we leave this behavior. If it was disabled
    // before this behavior started, it will be enabled anyway. If this becomes a problem, then we need to
    // count / track the requests to enable and disable like we do with track locking or reactionary behaviors
    robotInfo.EnableStopOnCliff(true);
  }
}
  
} // namespace Cozmo
} // namespace Anki
