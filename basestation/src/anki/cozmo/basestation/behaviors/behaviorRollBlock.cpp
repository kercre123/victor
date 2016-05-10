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
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/blockWorldFilter.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"

#define SET_STATE(s) SetState_internal(s, #s)

namespace Anki {
namespace Cozmo {

BehaviorRollBlock::BehaviorRollBlock(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _blockworldFilter( new BlockWorldFilter )
  , _robot(robot)
{
  SetDefaultName("RollBlock");

  // set up the filter we will use for finding blocks we care about
  _blockworldFilter->OnlyConsiderLatestUpdate(false);
  _blockworldFilter->SetFilterFcn( std::bind( &BehaviorRollBlock::FilterBlocks, this, std::placeholders::_1) );
}
  
bool BehaviorRollBlock::IsRunnableInternal(const Robot& robot) const
{
  // runnable if we have a block we'd like to roll
  return HasValidTargetBlock(robot);
}

float BehaviorRollBlock::EvaluateRunningScoreInternal(const Robot& robot) const
{
  // if we have requested, and are past the timeout, then we don't want to keep running
  float minScore = 0.0f;
  if( IsActing() ) {
    // while we are doing things, we really don't want to be interrupted
    minScore = 0.8f;
  }

  // otherwise, fall back to running score
  return std::max( minScore, EvaluateScoreInternal(robot) );
}


Result BehaviorRollBlock::InitInternal(Robot& robot)
{
  TransitionToSelectingTargetBlock(robot);
  return Result::RESULT_OK;
}
  
void BehaviorRollBlock::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}


void BehaviorRollBlock::TransitionToSelectingTargetBlock(Robot& robot)
{
  SET_STATE(State::SelectingTargetBlock);

  ObservableObject* closestObj = robot.GetBlockWorld().FindObjectClosestTo(robot.GetPose(), *_blockworldFilter);
  if( nullptr == closestObj ) {
    PRINT_NAMED_INFO("BehaviorRollBlock.TransitionToSelectingTargetBlock.NoValidBlock",
                     "couldn't find a target block, behavior is done");
  }
  else {
    _targetBlock = closestObj->GetID();

    StartActing(new TurnTowardsFaceWrapperAction(
                  robot,
                  new PlayAnimationGroupAction(robot, _initialAnimGroup)),                  
                &BehaviorRollBlock::TransitionToRollingBlock);
  }
}

void BehaviorRollBlock::TransitionToRollingBlock(Robot& robot)
{
  SET_STATE(State::RollingBlock);
  if( ! _targetBlock.IsSet() ) {
    PRINT_NAMED_ERROR("BehaviorRollBlock.TransitionToRollingBlock.NoBlockID",
                      "Transitioning to rolling block, but we don't have a valid block ID");
    // behavior will stop
  }
  else {

    DriveToRollObjectAction* rollAction = new DriveToRollObjectAction(robot, _targetBlock);
    rollAction->RollToUpright();

    StartActing(rollAction, 
                [&,this](const ExternalInterface::RobotCompletedAction& msg) {
                  if( msg.result == ActionResult::SUCCESS ) {
                    if( IsBlockInTargetRestingPosition( robot, _targetBlock ) ) {
                      // we rolled, but still need to roll again
                      TransitionToRollingBlock(robot);
                    }
                    else {
                      // we are done!
                      StartActing(new PlayAnimationGroupAction(robot, _successAnimGroup));
                    }
                  }
                  else if (msg.result == ActionResult::FAILURE_RETRY ) {
                    IActionRunner* animAction = nullptr;
                    switch(msg.completionInfo.Get_objectInteractionCompleted().result)
                    {
                      case ObjectInteractionResult::INCOMPLETE:
                      case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
                      {
                        animAction = new PlayAnimationGroupAction(robot, _realignAnimGroup);
                        break;
                      }
            
                      default: {
                        animAction = new PlayAnimationGroupAction(robot, _retryRollAnimGroup);
                        break;
                      }
                    }
                    assert(nullptr != animAction);

                    StartActing(animAction, &BehaviorRollBlock::TransitionToRollingBlock);
                  }
                  else {
                    // real failure or cancelation, just end the behavior
                    PRINT_NAMED_INFO("BehaviorRollBlock.RollBlockFailed",
                                     "roll block failed without retry, behavior ending");
                  }
                });
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
  _state = State::SelectingTargetBlock;
  _targetBlock.UnSet();
}

bool BehaviorRollBlock::FilterBlocks(ObservableObject* obj) const
{
  if( obj->GetFamily() != ObjectFamily::LightCube ||
      obj->GetPoseState() != ObservableObject::PoseState::Known || // only consider known, non-dirty blocks
      obj->IsMoving() ||
      !obj->IsRestingFlat() ||
      ( _robot.IsCarryingObject() && _robot.GetCarryingObject() == obj->GetID() ) || // ignore the block we are carrying
      !IsBlockInTargetRestingPosition(obj)
    ) {
    return false;
  }

  return true;
}

bool BehaviorRollBlock::IsBlockInTargetRestingPosition(const Robot& robot, ObjectID objectID) const
{
  return IsBlockInTargetRestingPosition( robot.GetBlockWorld().GetObjectByID(objectID) );
}

bool BehaviorRollBlock::IsBlockInTargetRestingPosition(const ObservableObject* obj) const
{
  if( nullptr == obj ) {
    // if we have a null object, assume that it is in the correct position, so we don't try to roll it
    return true;
  }

  if( !obj->IsRestingFlat() ) {
    return false;
  }

  AxisName upAxis = obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
  return upAxis != AxisName::Z_POS;
}

bool BehaviorRollBlock::HasValidTargetBlock(const Robot& robot) const
{
  // runnable if there are any valid target blocks
  std::vector<ObservableObject*> blocks;
  robot.GetBlockWorld().FindMatchingObjects(*_blockworldFilter, blocks);

  return ! blocks.empty();
}

}
}

