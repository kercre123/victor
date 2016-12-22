/**
 * File: behaviorRollBlock.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-05
 *
 * Description: This behavior rolls blocks when it see's them facing a direction specified in config
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorRollBlock.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviors/behaviorPutDownBlock.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

static const char* const kPutDownAnimGroupKey = "put_down_animation_group";
static const char* const kIsBlockRotationImportant = "isBlockRotationImportant";

CONSOLE_VAR(f32, kBRB_ScoreIncreaseForAction, "Behavior.RollBlock", 0.8f);
CONSOLE_VAR(f32, kBRB_MaxTowardFaceAngle_deg, "Behavior.RollBlock", 90.f);
CONSOLE_VAR(s32, kBRB_MaxRollRetries,         "Behavior.RollBlock", 1);
  
BehaviorRollBlock::BehaviorRollBlock(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _isBlockRotationImportant(true)
, _robot(robot)
{
  SetDefaultName("RollBlock");

  JsonTools::GetValueOptional(config,kPutDownAnimGroupKey,_putDownAnimTrigger);
  _isBlockRotationImportant = config.get(kIsBlockRotationImportant, true).asBool();
}

bool BehaviorRollBlock::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  UpdateTargetBlock(preReqData.GetRobot());
  
  return _targetBlock.IsSet() || IsActing();
}

Result BehaviorRollBlock::InitInternal(Robot& robot)
{
  if (robot.IsCarryingObject())
  {
    TransitionToSettingDownBlock(robot);
  }
  else
  {
    TransitionToPerformingAction(robot);
  }
  return Result::RESULT_OK;
}
  
void BehaviorRollBlock::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorRollBlock::StopInternalFromDoubleTap(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorRollBlock::UpdateTargetBlock(const Robot& robot) const
{
  if (!robot.IsCarryingObject())
  {
    using Intent = AIWhiteboard::ObjectUseIntention;
    const Intent intent = (_isBlockRotationImportant ? Intent::RollObjectWithAxisCheck : Intent::RollObjectNoAxisCheck);
    _targetBlock = _robot.GetAIComponent().GetWhiteboard().GetBestObjectForAction(intent);
  }
  else
  {
    const ObservableObject* carriedObj = robot.GetBlockWorld().GetLocatedObjectByID(robot.GetCarryingObject());
    if (nullptr != carriedObj && carriedObj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS)
    {
      _targetBlock = carriedObj->GetID();
    }
    else
    {
      _targetBlock.UnSet();
    }
  }
}

void BehaviorRollBlock::TransitionToSettingDownBlock(Robot& robot)
{
  DEBUG_SET_STATE(SettingDownBlock);
  
  if( _putDownAnimTrigger == AnimationTrigger::Count) {
    constexpr float kAmountToReverse_mm = 90.f;
    IActionRunner* actionsToDo = new CompoundActionSequential(robot, {
      new DriveStraightAction(robot, -kAmountToReverse_mm, DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
      new PlaceObjectOnGroundAction(robot)});
    StartActing(actionsToDo, [this](Robot& robot){TransitionToPerformingAction(robot);});
  }
  else {
    StartActing(new TriggerAnimationAction(robot, _putDownAnimTrigger),
                [this](Robot& robot) {
                  // use same logic as put down block behavior
                  StartActing(BehaviorPutDownBlock::CreateLookAfterPlaceAction(robot, false),
                              [this, &robot]() {
                                if(robot.IsCarryingObject()) {
                                  // No matter what, even if we didn't see the object we were
                                  // putting down for some reason, mark the robot as not carrying
                                  // anything so we don't get stuck in a loop of trying to put
                                  // something down (i.e. assume the object is no longer on our lift)
                                  // TODO: We should really be using some kind of PlaceOnGroundAction instead of raw animation (see COZMO-2192)
                                  PRINT_NAMED_WARNING("BehaviorRollBlock.TransitionToSettingDownBlock.DidNotSeeBlock",
                                                      "Forcibly setting carried objects as unattached (See COZMO-2192)");
                                  robot.SetCarriedObjectAsUnattached();
                                }
                                TransitionToPerformingAction(robot);
                              });
                });
  }
}

void BehaviorRollBlock::TransitionToPerformingAction(Robot& robot, bool isRetry)
{
  DEBUG_SET_STATE(PerformingAction);
  
  if( ! _targetBlock.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorRollBlock.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetName().c_str());
    return;
  }

  const Radians maxTurnToFaceAngle(_shouldStreamline ? 0 : DEG_TO_RAD(90));
  DriveToRollObjectAction* rollAction = new DriveToRollObjectAction(robot, _targetBlock,
                                                                    false, 0, false,
                                                                    maxTurnToFaceAngle);
  // always roll to upright, even if orientation isn't important (always prefer it to end upright, even if we
  // will run without it being upright, e.g. Sparks)
  rollAction->RollToUpright();
  rollAction->SetSayNameAnimationTrigger(AnimationTrigger::RollBlockPreActionNamedFace);
  rollAction->SetNoNameAnimationTrigger(AnimationTrigger::RollBlockPreActionUnnamedFace);
  
  auto preDockCallback = [this](Robot& robot) {
    // If this behavior uses a tapped object then prevent ReactToDoubleTap from interrupting
    if(RequiresObjectTapped())
    {
      robot.GetAIComponent().GetWhiteboard().SetSuppressReactToDoubleTap(true);
    }
  };
  
  rollAction->SetPreDockCallback(preDockCallback);
  
  RetryWrapperAction::RetryCallback retryCallback = [this, &robot, rollAction](const ExternalInterface::RobotCompletedAction& completion,
                                                                               const u8 retryCount,
                                                                               AnimationTrigger& retryAnimTrigger)
  {
    retryAnimTrigger = AnimationTrigger::RollBlockRetry;
  
    // Don't turn towards the face when retrying
    rollAction->DontTurnTowardsFace();
  
    // Only try to use another preAction pose if we aren't using an approach angle otherwise there is only
    // one preAction pose to roll the object upright and the roll action failed due to not seeing the object
    if(!rollAction->GetUseApproachAngle() &&
       completion.result == ActionResult::VISUAL_OBSERVATION_FAILED)
    {
      // Use a different preAction pose if we are retrying
      rollAction->GetDriveToObjectAction()->SetGetPossiblePosesFunc([this, rollAction](ActionableObject* object,
                                                                                       std::vector<Pose3d>& possiblePoses,
                                                                                       bool& alreadyInPosition)
      {
        return IBehavior::UseSecondClosestPreActionPose(rollAction->GetDriveToObjectAction(),object, possiblePoses, alreadyInPosition);
      });
    }
    
    switch(completion.result)
    {
      case ActionResult::LAST_PICK_AND_PLACE_FAILED:
      {
        retryAnimTrigger = AnimationTrigger::RollBlockRetry;
        break;
      }
      
      default:
      {
        retryAnimTrigger = AnimationTrigger::RollBlockRealign;
        break;
      }
    }
  
    return true;
  };
  
  RetryWrapperAction* action = new RetryWrapperAction(robot,
                                                      rollAction,
                                                      retryCallback,
                                                      kBRB_MaxRollRetries);

  // Roll the object and then look at a person
  StartActing(action,
              [&,this](const ExternalInterface::RobotCompletedAction& msg) {
                if(msg.result == ActionResult::SUCCESS)
                {
                  if(!_shouldStreamline){
                    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::RollBlockSuccess));
                  }
                  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
                  BehaviorObjectiveAchieved(BehaviorObjective::BlockRolled);
                }
                else
                {
                  PRINT_NAMED_INFO("BehaviorRollBlock.FailedAbort", "Failed to verify roll");
                  
                  const ObservableObject* failedObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlock);
                  if(failedObject){
                    robot.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject, AIWhiteboard::ObjectUseAction::RollOrPopAWheelie);
                  }
                }
              });
  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
}
  
  
void BehaviorRollBlock::SetupRetryAction(Robot& robot, const ExternalInterface::RobotCompletedAction& msg)
{
  // Pick which retry animation, if any, to play. Then try performing the action again,
  // with "isRetry" set to true.
  
  IActionRunner* animAction = nullptr;
  switch(msg.result)
  {
    case ActionResult::LAST_PICK_AND_PLACE_FAILED:
    {
      animAction = new TriggerAnimationAction(robot, AnimationTrigger::RollBlockRetry);
      break;
    }
      
    default: {
      animAction = new TriggerAnimationAction(robot, AnimationTrigger::RollBlockRealign);
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

void BehaviorRollBlock::ResetBehavior(Robot& robot)
{
  _targetBlock.UnSet();
}

}
}
