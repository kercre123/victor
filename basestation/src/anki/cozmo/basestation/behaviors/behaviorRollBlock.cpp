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

#include "anki/cozmo/basestation/behaviors/behaviorRollBlock.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviors/behaviorPutDownBlock.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/blockWorldFilter.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"


#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

static const char* const kPutDownAnimGroupKey = "put_down_animation_group";

CONSOLE_VAR(f32, kBRB_ScoreIncreaseForAction, "Behavior.RollBlock", 0.8f);
CONSOLE_VAR(f32, kBRB_MaxTowardFaceAngle_deg, "Behavior.RollBlock", 90.f);
CONSOLE_VAR(s32, kBRB_MaxRollRetries,         "Behavior.RollBlock", 1);
  
BehaviorRollBlock::BehaviorRollBlock(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _blockworldFilter( new BlockWorldFilter )
  , _robot(robot)
{
  SetDefaultName("RollBlock");

  _putDownAnimGroup = config.get(kPutDownAnimGroupKey, "").asCString();
  
  // set up the filter we will use for finding blocks we care about
  _blockworldFilter->OnlyConsiderLatestUpdate(false);
  _blockworldFilter->SetFilterFcn( std::bind( &BehaviorRollBlock::FilterBlocks, this, std::placeholders::_1) );
}

bool BehaviorRollBlock::IsRunnableInternal(const Robot& robot) const
{
  UpdateTargetBlock(robot);
  
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
    TransitionToReactingToBlock(robot);
  }
  return Result::RESULT_OK;
}
  
void BehaviorRollBlock::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorRollBlock::UpdateTargetBlock(const Robot& robot) const
{
  if (!robot.IsCarryingObject())
  {
    ObservableObject* closestObj = robot.GetBlockWorld().FindObjectClosestTo(robot.GetPose(), *_blockworldFilter);
    if( nullptr != closestObj ) {
      _targetBlock = closestObj->GetID();
    }
    else {
      _targetBlock.UnSet();
    }
  }
  else
  {
    const ObservableObject* carriedObj = robot.GetBlockWorld().GetActiveObjectByID(robot.GetCarryingObject());
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

bool BehaviorRollBlock::FilterBlocks(ObservableObject* obj) const
{
  return (!obj->IsPoseStateUnknown() &&
          _robot.CanPickUpObjectFromGround(*obj) &&
          obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS);
}
  
void BehaviorRollBlock::TransitionToSettingDownBlock(Robot& robot)
{
  SET_STATE(SettingDownBlock);

  if( _putDownAnimGroup.empty() ) {
    constexpr float kAmountToReverse_mm = 90.f;
    IActionRunner* actionsToDo = new CompoundActionSequential(robot, {
        new DriveStraightAction(robot, -kAmountToReverse_mm, DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
        new PlaceObjectOnGroundAction(robot)});
    StartActing(actionsToDo, &BehaviorRollBlock::TransitionToReactingToBlock);
  }
  else {
    StartActing(new PlayAnimationGroupAction(robot, _putDownAnimGroup),
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
                                TransitionToReactingToBlock(robot);
                              });
                });
  }
}

void BehaviorRollBlock::TransitionToReactingToBlock(Robot& robot)
{
  SET_STATE(ReactingToBlock);

  if( robot.IsCarryingObject() ) {
    PRINT_NAMED_ERROR("BehaviorRollBlock.ReactWhileHolding",
                      "block should be put down at this point. Bailing from behavior");
    _targetBlock.UnSet();
    return;
  }

  if( !_initialAnimGroup.empty() ) {
    // Turn towards the object and then react to it before performing the roll action
    StartActing(new CompoundActionSequential(robot, {
                  new TurnTowardsObjectAction(robot, _targetBlock, PI_F),
                  new PlayAnimationGroupAction(robot, _initialAnimGroup),
                }),
                [this,&robot]{ this->TransitionToPerformingAction(robot); });
  }
  else {
    TransitionToPerformingAction(robot);
  }
}

void BehaviorRollBlock::TransitionToPerformingAction(Robot& robot, bool isRetry)
{
  SET_STATE(PerformingAction);
  if( ! _targetBlock.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorRollBlock.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetName().c_str());
    return;
  }
  
  if(isRetry) {
    ++_numRollActionRetries;
    PRINT_NAMED_INFO("BehaviorRollBlock.TransitionToPerformingAction.Retrying",
                     "Retry %d of %d", _numRollActionRetries, kBRB_MaxRollRetries);
  } else {
    _numRollActionRetries = 0;
  }

  // Only turn towards face if this is _not_ a retry
  const Radians maxTurnToFaceAngle( (isRetry ? 0 : DEG_TO_RAD(90)) );
  const bool sayName = true;
  DriveToRollObjectAction* rollAction = new DriveToRollObjectAction(robot, _targetBlock,
                                                                    false, 0, false,
                                                                    maxTurnToFaceAngle, sayName);
  rollAction->RollToUpright();
  
  WaitForLambdaAction* waitAction = new WaitForLambdaAction(robot, [this](Robot& robot) {
    auto block = robot.GetBlockWorld().GetActiveObjectByID(_targetBlock);
    if(nullptr == block || !block->IsMoving()) {
      return true;
    } else {
      return false;
    }
  });
  
  CompoundActionSequential* action = new CompoundActionSequential(robot, {
    rollAction,
    new TurnTowardsLastFacePoseAction(robot, PI_F, true),
    waitAction,
  });
  
  action->SetProxyTag(rollAction->GetTag());
  
  // Roll the object and then look at a person
  StartActing(action,
              [&,this](const ExternalInterface::RobotCompletedAction& msg) {
                switch(msg.result)
                {
                  case ActionResult::SUCCESS:
                    if( !_successAnimGroup.empty() ) {
                      StartActing(new PlayAnimationGroupAction(robot, _successAnimGroup));
                    }
                    break;
                  
                  case ActionResult::FAILURE_RETRY:
                    if(_numRollActionRetries < kBRB_MaxRollRetries)
                    {
                      SetupRetryAction(robot, msg);
                      break;
                    }
                    
                    // else: too many retries, fall through to failure abort
                    
                  case ActionResult::FAILURE_ABORT:
                    // assume that failure is because we didn't visually verify (although it could be due to an
                    // error)
                    PRINT_NAMED_INFO("BehaviorRollBlock.FailedAbort",
                                     "Failed to verify roll, searching for block");
                    StartActing(new SearchSideToSideAction(robot),
                                [this](Robot& robot) {
                                  if( IsRunnable(robot) ) {
                                    TransitionToPerformingAction(robot);
                                  }
                                  // TODO:(bn) if we actually succeeded here, we should play the success anim,
                                  // or at least a recognition anim
                                });
                    break;
                    
                  default:
                    // other failure, just end
                    PRINT_NAMED_INFO("BehaviorRollBlock.FailedRollAction",
                                     "action failed with %s, behavior ending",
                                     EnumToString(msg.result));
                } // switch(msg.result)
                
              });
  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
}
  
  
void BehaviorRollBlock::SetupRetryAction(Robot& robot, const ExternalInterface::RobotCompletedAction& msg)
{
  // Pick which retry animation, if any, to play. Then try performing the action again,
  // with "isRetry" set to true.
  
  IActionRunner* animAction = nullptr;
  switch(msg.completionInfo.Get_objectInteractionCompleted().result)
  {
    case ObjectInteractionResult::INCOMPLETE:
    case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
    {
      if( ! _realignAnimGroup.empty() ) {
        animAction = new PlayAnimationGroupAction(robot, _realignAnimGroup);
      }
      break;
    }
      
    default: {
      if( ! _retryActionAnimGroup.empty() ) {
        animAction = new PlayAnimationGroupAction(robot, _retryActionAnimGroup);
      }
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

  
void BehaviorRollBlock::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorRollBlock.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}

void BehaviorRollBlock::ResetBehavior(Robot& robot)
{
  _state = State::ReactingToBlock;
  _targetBlock.UnSet();
}

}
}
