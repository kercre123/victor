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
  
  return _targetBlock.IsSet();
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
  return _robot.CanPickUpObjectFromGround(*obj) &&
    obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS;
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
                  StartActing(BehaviorPutDownBlock::CreateLookAfterPlaceAction(robot),
                               &BehaviorRollBlock::TransitionToReactingToBlock);
                });
  }
}

void BehaviorRollBlock::TransitionToReactingToBlock(Robot& robot)
{
  SET_STATE(ReactingToBlock);

  if( robot.IsCarryingObject() ) {
    PRINT_NAMED_ERROR("BehaviorRollBlock.ReactWhileHolding",
                      "block should be put down at this point. Bailing from behavior");
    return;
  }

  if( !_initialAnimGroup.empty() ) {
    StartActing(new TurnTowardsFaceWrapperAction(
                  robot,
                  new PlayAnimationGroupAction(robot, _initialAnimGroup)),                  
                &BehaviorRollBlock::TransitionToPerformingAction);
  }
  else {
    TransitionToPerformingAction(robot);
  }
}

void BehaviorRollBlock::TransitionToPerformingAction(Robot& robot)
{
  SET_STATE(PerformingAction);
  if( ! _targetBlock.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorRollBlock.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetName().c_str());
    return;
  }

  DriveToRollObjectAction* action = new DriveToRollObjectAction(robot, _targetBlock);
  action->RollToUpright();

  StartActing(action,
              [&,this](const ExternalInterface::RobotCompletedAction& msg) {
                if( msg.result == ActionResult::SUCCESS ) {
                  // check if we want to run again

                  // TODO:(bn) this sometimes fails because the block is still reporting that it's moving, but
                  // in a few ticks it won't be. We should consider adding a delay here, or better yet, having
                  // an extra state where if a cube is valid for other reasons, but is moving or not flat, we
                  // "track" it for a while
                  if( IsRunnable(robot) ) {
                    TransitionToPerformingAction(robot);
                  }
                  else {
                    // play success animation, but then double check if we are really done or not
                    auto animCompleteBlock = [this](Robot& robot) {
                      if( IsRunnable(robot) ) {
                        TransitionToPerformingAction(robot);
                      }
                    };

                    if( !_successAnimGroup.empty() ) {
                      StartActing(new PlayAnimationGroupAction(robot, _successAnimGroup), animCompleteBlock);
                    }
                    else {
                      animCompleteBlock(robot);
                    }
                  }
                }
                else if( msg.result == ActionResult::FAILURE_RETRY ) {
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
                    StartActing(animAction, &BehaviorRollBlock::TransitionToPerformingAction);
                  }
                  else {
                    TransitionToPerformingAction(robot);
                  }
                }
                else if( msg.result == ActionResult::FAILURE_ABORT ) {
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
                }
                else {
                  // other failure, just end
                  PRINT_NAMED_INFO("BehaviorRollBlock.FailedActionNoRetry",
                                   "action failed without retry, behavior ending");
                }
              });
  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
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
