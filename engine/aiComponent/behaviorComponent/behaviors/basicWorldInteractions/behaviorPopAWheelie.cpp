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
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {

namespace{
CONSOLE_VAR(f32, kBPW_ScoreIncreaseForAction, "Behavior.PopAWheelie", 0.8f);
CONSOLE_VAR(s32, kBPW_MaxRetries,         "Behavior.PopAWheelie", 1);

} // end namespace

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPopAWheelie::BehaviorPopAWheelie(const Json::Value& config)
: ICozmoBehavior(config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPopAWheelie::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  UpdateTargetBlock(behaviorExternalInterface);
  
  return _targetBlock.IsSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPopAWheelie::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(!ShouldStreamline() && _lastBlockReactedTo != _targetBlock){
    TransitionToReactingToBlock(behaviorExternalInterface);
  }else{
    TransitionToPerformingAction(behaviorExternalInterface);
  }
  
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  ResetBehavior(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::UpdateTargetBlock(BehaviorExternalInterface& behaviorExternalInterface) const
{
  _targetBlock = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache().
       GetBestObjectForIntention(ObjectInteractionIntention::PopAWheelieOnObject);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToReactingToBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(ReactingToBlock);

  // Turn towards the object and then react to it before performing the pop a wheelie action
  DelegateIfInControl(new CompoundActionSequential({
      new TurnTowardsObjectAction(_targetBlock),
      new TriggerLiftSafeAnimationAction(AnimationTrigger::PopAWheelieInitial),
    }),
    [this, &behaviorExternalInterface](const ActionResult& res){
      if(res == ActionResult::SUCCESS)
      {
        _lastBlockReactedTo = _targetBlock;
      }
      TransitionToPerformingAction(behaviorExternalInterface);
    });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToPerformingAction(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(PerformingAction);
  TransitionToPerformingAction(behaviorExternalInterface,false);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToPerformingAction(BehaviorExternalInterface& behaviorExternalInterface, bool isRetry)
{
  if( ! _targetBlock.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorPopAWheelie.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetIDStr().c_str());
    return;
  }
  
  if(isRetry) {
    ++_numPopAWheelieActionRetries;
    PRINT_NAMED_INFO("BehaviorPopAWheelie.TransitionToPerformingAction.Retrying",
                     "Retry %d of %d", _numPopAWheelieActionRetries, kBPW_MaxRetries);
  } else {
    _numPopAWheelieActionRetries = 0;
  }
  
  // Only turn towards face if this is _not_ a retry
  const Radians maxTurnToFaceAngle( (isRetry || ShouldStreamline() ? 0 : DEG_TO_RAD(90)) );

  DriveToPopAWheelieAction* goPopAWheelie = new DriveToPopAWheelieAction(_targetBlock,
                                                                         false,
                                                                         0,
                                                                         false,
                                                                         maxTurnToFaceAngle);
  goPopAWheelie->SetSayNameAnimationTrigger(AnimationTrigger::PopAWheeliePreActionNamedFace);
  goPopAWheelie->SetNoNameAnimationTrigger(AnimationTrigger::PopAWheeliePreActionUnnamedFace);
  
  // once we get to the predock pose, before docking, disable the cliff sensor and associated reactions so
  // that we play the correct animation instead of getting interrupted)  
  auto disableCliff = [this](Robot& robot) {
    // tell the robot not to stop the current action / animation if the cliff sensor fires
    _hasDisabledcliff = true;
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));
  };
  goPopAWheelie->SetPreDockCallback(disableCliff);


  DelegateIfInControl(goPopAWheelie,
              [&, this](const ExternalInterface::RobotCompletedAction& msg) {                
                switch(IActionRunner::GetActionResultCategory(msg.result))
                {
                  case ActionResultCategory::SUCCESS:
                  {
                    _lastBlockReactedTo.UnSet();
                    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::SuccessfulWheelie));
                    BehaviorObjectiveAchieved(BehaviorObjective::PoppedWheelie);
                    NeedActionCompleted();
                    break;
                  }
                  case ActionResultCategory::RETRY:
                  {
                    if(_numPopAWheelieActionRetries < kBPW_MaxRetries)
                    {
                      SetupRetryAction(behaviorExternalInterface, msg);
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
                    
                    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
                    // be removed
                    Robot& robot = behaviorExternalInterface.GetRobot();
                    
                    // mark the block as inaccessible
                    const ObservableObject* failedObject = failedObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlock);
                    if(failedObject){
                      robot.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
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
void BehaviorPopAWheelie::SetupRetryAction(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotCompletedAction& msg)
{
  //Ensure that the closest block is selected
  UpdateTargetBlock(behaviorExternalInterface);
  
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
    DelegateIfInControl(animAction, [this,&behaviorExternalInterface]() {
      this->TransitionToPerformingAction(behaviorExternalInterface, true);
    });
  }
  else {
    TransitionToPerformingAction(behaviorExternalInterface, true);
  }
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::ResetBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  _targetBlock.UnSet();

  if( _hasDisabledcliff ) {
    _hasDisabledcliff = false;
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    // NOTE: assumes that we want the cliff to be re-enabled when we leave this behavior. If it was disabled
    // before this behavior started, it will be enabled anyway. If this becomes a problem, then we need to
    // count / track the requests to enable and disable like we do with track locking or reactionary behaviors
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(true)));
  }
}
  
} // namespace Cozmo
} // namespace Anki
