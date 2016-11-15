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

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPopAWheelie.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "util/console/consoleInterface.h"



namespace Anki {
namespace Cozmo {

CONSOLE_VAR(f32, kBPW_ScoreIncreaseForAction, "Behavior.PopAWheelie", 0.8f);
CONSOLE_VAR(f32, kBPW_MaxTowardFaceAngle_deg, "Behavior.PopAWheelie", 90.f);
CONSOLE_VAR(s32, kBPW_MaxRetries,         "Behavior.PopAWheelie", 1);

BehaviorPopAWheelie::BehaviorPopAWheelie(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _robot(robot)
{
  SetDefaultName("PopAWheelie");
}

bool BehaviorPopAWheelie::IsRunnableInternal(const Robot& robot) const
{
  UpdateTargetBlock(robot);
  
  return _targetBlock.IsSet();
}

Result BehaviorPopAWheelie::InitInternal(Robot& robot)
{
  if(!_shouldStreamline){
    TransitionToReactingToBlock(robot);
  }else{
    TransitionToPerformingAction(robot);
  }
  
  return Result::RESULT_OK;
}

void BehaviorPopAWheelie::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorPopAWheelie::StopInternalFromDoubleTap(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorPopAWheelie::UpdateTargetBlock(const Robot& robot) const
{
  _targetBlock = _robot.GetBehaviorManager().GetWhiteboard().GetBestObjectForAction(AIWhiteboard::ObjectUseIntention::PopAWheelieOnObject);
}

void BehaviorPopAWheelie::TransitionToReactingToBlock(Robot& robot)
{
  DEBUG_SET_STATE(ReactingToBlock);

  // Turn towards the object and then react to it before performing the pop a wheelie action
  StartActing(new CompoundActionSequential(robot, {
    new TurnTowardsObjectAction(robot, _targetBlock, PI_F),
    new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PopAWheelieInitial),
  }),
  &BehaviorPopAWheelie::TransitionToPerformingAction);
}

void BehaviorPopAWheelie::TransitionToPerformingAction(Robot& robot)
{
  DEBUG_SET_STATE(PerformingAction);
  TransitionToPerformingAction(robot,false);
}
  
void BehaviorPopAWheelie::TransitionToPerformingAction(Robot& robot, bool isRetry)
{
  if( ! _targetBlock.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorPopAWheelie.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetName().c_str());
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
  const Radians maxTurnToFaceAngle( (isRetry || _shouldStreamline ? 0 : DEG_TO_RAD(90)) );
  DriveToPopAWheelieAction* goPopAWheelie = new DriveToPopAWheelieAction(robot,
                                                                         _targetBlock,
                                                                         false,
                                                                         0,
                                                                         false,
                                                                         maxTurnToFaceAngle);
  goPopAWheelie->SetSayNameAnimationTrigger(AnimationTrigger::PopAWheeliePreActionNamedFace);
  goPopAWheelie->SetNoNameAnimationTrigger(AnimationTrigger::PopAWheeliePreActionUnnamedFace);

  // once we get to the predock pose, before docking, disable the cliff sensor and associated reactions so
  // that we play the correct animation instead of getting interrupted)  
  auto disableCliff = [this](Robot& robot) {
    // disable reactions we don't want
    SmartDisableReactionaryBehavior(BehaviorType::ReactToCliff);
    SmartDisableReactionaryBehavior(BehaviorType::ReactToPickup);
    SmartDisableReactionaryBehavior(BehaviorType::ReactToRobotOnBack);
    SmartDisableReactionaryBehavior(BehaviorType::ReactToCubeMoved);
    SmartDisableReactionaryBehavior(BehaviorType::ReactToUnexpectedMovement);
    SmartDisableReactionaryBehavior(BehaviorType::ReactToReturnedToTreads);

    // tell the robot not to stop the current action / animation if the cliff sensor fires
    _hasDisabledcliff = true;
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));
    
    // If this behavior uses a tapped object then prevent ReactToDoubleTap from interrupting
    if(RequiresObjectTapped())
    {
      robot.GetBehaviorManager().GetWhiteboard().SetSuppressReactToDoubleTap(true);
    }
  };
  goPopAWheelie->SetPreDockCallback(disableCliff);


  StartActing(goPopAWheelie,
              [&,this](const ExternalInterface::RobotCompletedAction& msg) {
                if(msg.result != ActionResult::SUCCESS){
                  SmartReEnableReactionaryBehavior(BehaviorType::ReactToRobotOnBack);
                }
                
                switch(msg.result)
                {
                  case ActionResult::SUCCESS:
                  {
                    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::SuccessfulWheelie));
                    BehaviorObjectiveAchieved(BehaviorObjective::PoppedWheelie);
                    break;
                  }
                  case ActionResult::FAILURE_RETRY:
                  {
                    if(_numPopAWheelieActionRetries < kBPW_MaxRetries)
                    {
                      SetupRetryAction(robot, msg);
                      break;
                    }
                    
                    // else: too many retries, fall through to failure abort
                  }
                  case ActionResult::FAILURE_ABORT:
                  {
                    // assume that failure is because we didn't visually verify (although it could be due to an
                    // error)
                    PRINT_NAMED_INFO("BehaviorPopAWheelie.FailedAbort",
                                     "Failed to verify pop, searching for block");
                    
                    // mark the block as inaccessible
                    const ObservableObject* failedObject = failedObject = robot.GetBlockWorld().GetObjectByID(_targetBlock);
                    if(failedObject){
                      robot.GetBehaviorManager().GetWhiteboard().SetFailedToUse(*failedObject, AIWhiteboard::ObjectUseAction::RollOrPopAWheelie);
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
  
  IncreaseScoreWhileActing( kBPW_ScoreIncreaseForAction );
}


void BehaviorPopAWheelie::SetupRetryAction(Robot& robot, const ExternalInterface::RobotCompletedAction& msg)
{
  //Ensure that the closest block is selected
  UpdateTargetBlock(robot);
  
  // Pick which retry animation, if any, to play. Then try performing the action again,
  // with "isRetry" set to true.
  
  IActionRunner* animAction = nullptr;
  switch(msg.completionInfo.Get_objectInteractionCompleted().result)
  {
    case ObjectInteractionResult::INCOMPLETE:
    case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
    {
      animAction = new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PopAWheelieRealign);
      break;
    }
      
    default: {
      animAction = new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PopAWheelieRetry);
      break;
    }
  }
  
  if( nullptr != animAction ) {
    StartActing(animAction, [this,&robot]() {
      this->TransitionToPerformingAction(robot, true);
    });
  }
  else {
    TransitionToPerformingAction(robot, true);
  }
  
}

void BehaviorPopAWheelie::ResetBehavior(Robot& robot)
{
  _targetBlock.UnSet();

  if( _hasDisabledcliff ) {
    _hasDisabledcliff = false;
    // NOTE: assumes that we want the cliff to be re-enabled when we leave this behavior. If it was disabled
    // before this behavior started, it will be enabled anyway. If this becomes a problem, then we need to
    // count / track the requests to enable and disable like we do with track locking or reactionary behaviors
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(true)));
  }
}

}
}
