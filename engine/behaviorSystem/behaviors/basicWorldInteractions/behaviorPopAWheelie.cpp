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

#include "engine/behaviorSystem/behaviors/basicWorldInteractions/behaviorPopAWheelie.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersPopAWheelieArray = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 true},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  true},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           true},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersPopAWheelieArray),
              "Reaction triggers duplicate or non-sequential");

} // end namespace

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPopAWheelie::BehaviorPopAWheelie(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _robot(robot)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPopAWheelie::IsRunnableInternal(const Robot& robot) const
{
  UpdateTargetBlock(robot);
  
  return _targetBlock.IsSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPopAWheelie::InitInternal(Robot& robot)
{
  if(!ShouldStreamline() && _lastBlockReactedTo != _targetBlock){
    TransitionToReactingToBlock(robot);
  }else{
    TransitionToPerformingAction(robot);
  }
  
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::UpdateTargetBlock(const Robot& robot) const
{
  _targetBlock = _robot.GetAIComponent().GetObjectInteractionInfoCache().
       GetBestObjectForIntention(ObjectInteractionIntention::PopAWheelieOnObject);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToReactingToBlock(Robot& robot)
{
  DEBUG_SET_STATE(ReactingToBlock);

  // Turn towards the object and then react to it before performing the pop a wheelie action
  StartActing(new CompoundActionSequential(robot, {
      new TurnTowardsObjectAction(robot, _targetBlock),
      new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PopAWheelieInitial),
    }),
    [this, &robot](const ActionResult& res){
      if(res == ActionResult::SUCCESS)
      {
        _lastBlockReactedTo = _targetBlock;
      }
      TransitionToPerformingAction(robot);
    });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToPerformingAction(Robot& robot)
{
  DEBUG_SET_STATE(PerformingAction);
  TransitionToPerformingAction(robot,false);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::TransitionToPerformingAction(Robot& robot, bool isRetry)
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
    SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersPopAWheelieArray);
    
    // tell the robot not to stop the current action / animation if the cliff sensor fires
    _hasDisabledcliff = true;
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));
  };
  goPopAWheelie->SetPreDockCallback(disableCliff);


  StartActing(goPopAWheelie,
              [&, this](const ExternalInterface::RobotCompletedAction& msg) {
                if(msg.result != ActionResult::SUCCESS){
                  this->SmartRemoveDisableReactionsLock(GetIDStr());
                }
                
                switch(IActionRunner::GetActionResultCategory(msg.result))
                {
                  case ActionResultCategory::SUCCESS:
                  {
                    _lastBlockReactedTo.UnSet();
                    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::SuccessfulWheelie));
                    BehaviorObjectiveAchieved(BehaviorObjective::PoppedWheelie);
                    NeedActionCompleted();
                    break;
                  }
                  case ActionResultCategory::RETRY:
                  {
                    if(_numPopAWheelieActionRetries < kBPW_MaxRetries)
                    {
                      SetupRetryAction(robot, msg);
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
  
  IncreaseScoreWhileActing( kBPW_ScoreIncreaseForAction );
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPopAWheelie::SetupRetryAction(Robot& robot, const ExternalInterface::RobotCompletedAction& msg)
{
  //Ensure that the closest block is selected
  UpdateTargetBlock(robot);
  
  // Pick which retry animation, if any, to play. Then try performing the action again,
  // with "isRetry" set to true.
  
  IActionRunner* animAction = nullptr;
  switch(msg.result)
  {
    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
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
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
  
} // namespace Cozmo
} // namespace Anki
