/**
 * File: behaviorStackBlocks.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-05-11
 *
 * Description: Behavior to pick up one cube and stack it on another
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorStackBlocks.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/blockWorldFilter.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(f32, kBSB_ScoreIncreaseForAction, "Behavior.StackBlocks", 0.8f);
CONSOLE_VAR(f32, kBSB_MaxTurnTowardsFaceBeforePickupAngle_deg, "Behavior.StackBlocks", 90.f);
CONSOLE_VAR(s32, kBSB_MaxNumPickupRetries, "Behavior.StackBlocks", 2);
CONSOLE_VAR(f32, kBSB_SamePreactionPoseDistThresh_mm, "Behavior.StackBlocks", 30.f);
CONSOLE_VAR(f32, kBSB_SamePreactionPoseAngleThresh_deg, "Behavior.StackBlocks", 45.f);
  
BehaviorStackBlocks::BehaviorStackBlocks(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _blockworldFilterForTop( new BlockWorldFilter )
  , _blockworldFilterForBottom( new BlockWorldFilter )
  , _robot(robot)
  , _waitingForBlockToBeValid(false)
{
  SetDefaultName("StackBlocks");

  // set up the filter we will use for finding blocks we care about
  _blockworldFilterForTop->OnlyConsiderLatestUpdate(false);
  _blockworldFilterForTop->SetFilterFcn( std::bind( &BehaviorStackBlocks::FilterBlocksForTop,
                                                    this,
                                                    std::placeholders::_1) );

  _blockworldFilterForBottom->OnlyConsiderLatestUpdate(false);
  _blockworldFilterForBottom->SetFilterFcn( std::bind( &BehaviorStackBlocks::FilterBlocksForBottom,
                                                       this,
                                                       std::placeholders::_1) );
  
  SubscribeToTags({
    EngineToGameTag::RobotOffTreadsStateChanged
  });
}

bool BehaviorStackBlocks::IsRunnableInternal(const Robot& robot) const
{
  // don't change blocks while we're running
  if( !IsRunning() ) {
    UpdateTargetBlocks(robot);
  }
  
  return _targetBlockBottom.IsSet() && _targetBlockTop.IsSet();
}

Result BehaviorStackBlocks::InitInternal(Robot& robot)
{
  _waitingForBlockToBeValid = false;
  TransitionToPickingUpBlock(robot);
  return Result::RESULT_OK;
}
  
void BehaviorStackBlocks::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorStackBlocks::UpdateTargetBlocks(const Robot& robot) const
{

  // if we've got a cube in our lift, use that for top
  const ObjectID lastTopID = _targetBlockTop;
  _targetBlockTop.UnSet();
  if( robot.IsCarryingObject() ) {
    const ObservableObject* carriedObject = robot.GetBlockWorld().GetObjectByID( robot.GetCarryingObject() );

    if( nullptr != carriedObject ) {
      const bool upAxisOk = ! robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::RollCube) ||
        carriedObject->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;

      if( upAxisOk ) {
        _targetBlockTop = carriedObject->GetID();
      }
    }
  }

  if( ! _targetBlockTop.IsSet() ) {
  
    const ObservableObject* topObject = robot.GetBlockWorld().FindObjectClosestTo(robot.GetPose(),
                                                                            *_blockworldFilterForTop);
    if( nullptr != topObject ) {
      _targetBlockTop = topObject->GetID();
    }
  }

  if( lastTopID.IsSet() && ! _targetBlockTop.IsSet() ) {
    const ObservableObject* lastTop = robot.GetBlockWorld().GetObjectByID(lastTopID);
    if( nullptr == lastTop ) {
      PRINT_NAMED_DEBUG("BehaviorStackBlocks.UpdateTargets.LostTopBlock.null",
                        "last top (%d) must have been deleted",
                        lastTopID.GetValue());
    }
    else {
      PrintCubeDebug("BehaviorStackBlocks.UpdateTargets.LostTopBlock", lastTop);
    }
  }

  const ObservableObject* bottomObject = robot.GetBlockWorld().FindObjectClosestTo(robot.GetPose(),
                                                                             *_blockworldFilterForBottom);
  if( nullptr != bottomObject ) {
    _targetBlockBottom = bottomObject->GetID();
  }
  else {
    if( _targetBlockBottom.IsSet() ) {
      const ObservableObject* oldBottom = robot.GetBlockWorld().GetObjectByID(_targetBlockBottom);
      if( nullptr == oldBottom ) {
        PRINT_NAMED_DEBUG("BehaviorStackBlocks.UpdateTargets.LostBottomBlock.null",
                          "last bottom (%d) must have been deleted",
                          _targetBlockBottom.GetValue());
      }
      else {
        PrintCubeDebug("BehaviorStackBlocks.UpdateTargets.LostBottomBlock", oldBottom);
      }
    }
    _targetBlockBottom.UnSet();
  }
}

bool BehaviorStackBlocks::FilterBlocksHelper(const ObservableObject* obj) const
{
  const bool upAxisOk = ! _robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::RollCube) ||
    obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;

  return obj->GetFamily() == ObjectFamily::LightCube && !obj->IsPoseStateUnknown() && upAxisOk;
}

bool BehaviorStackBlocks::FilterBlocksForTop(const ObservableObject* obj) const
{
  return FilterBlocksHelper(obj) && _robot.CanPickUpObject(*obj);
}

bool BehaviorStackBlocks::FilterBlocksForBottom(const ObservableObject* obj) const
{
  // top gets picked first, so can't pick top here
  return obj->GetID() != _targetBlockTop && FilterBlocksHelper(obj) && _robot.CanStackOnTopOfObject( *obj );
}

bool BehaviorStackBlocks::AreBlocksAreStillValid(const Robot& robot)
{
  if( !_targetBlockTop.IsSet() || !_targetBlockBottom.IsSet() ) {

    if( !_waitingForBlockToBeValid) {
      PRINT_NAMED_INFO("BehaviorStackBlocks.InvalidBlock.BlocksNoLongerSet",
                       "one of the blocks isn't set");
    }
    
    return false;
  }

  // if the top block is being carried, assume it's valid (if it matches what top should be). Otherwise check it
  if( robot.IsCarryingObject() && robot.GetCarryingObject() != _targetBlockTop ) {
    PRINT_NAMED_INFO("BehaviorStackBlocks.InvalidBlock.CarryingWrongObject",
                     "robot is carrying object %d, but %d is supposed to be the top",
                     robot.GetCarryingObject().GetValue(),
                     _targetBlockTop.GetValue());
    return false;
  }
      
  if( !robot.IsCarryingObject() ) {
    const ObservableObject* topObject = robot.GetBlockWorld().GetObjectByID(_targetBlockTop);
    if( topObject == nullptr ) {
      PRINT_NAMED_INFO("BehaviorStackBlocks.InvalidBlock.BlockDeleted",
                       "target block %d has no pointer in blockworld",
                       _targetBlockTop.GetValue());
      _targetBlockTop.UnSet();
      return false;
    }

    if( ! FilterBlocksForTop( topObject ) ) {
      PRINT_NAMED_INFO("BehaviorStackBlocks.InvalidBlock.TopFailedFilter",
                       "top block failed it's filter");

      PrintCubeDebug("BehaviorStackBlocks.InvalidBlock.TopFailedFilter.Debug", topObject);
      return false;
    }
  }
  
  const ObservableObject* bottomObject = robot.GetBlockWorld().GetObjectByID(_targetBlockBottom);
  if( bottomObject == nullptr ) {
    PRINT_NAMED_INFO("BehaviorStackBlocks.BlockDeleted",
                     "target block %d has no pointer in blockworld",
                     _targetBlockBottom.GetValue());
    _targetBlockBottom.UnSet();
    return false;
  }

  if( ! FilterBlocksForBottom( bottomObject ) ) {
    PRINT_NAMED_INFO("BehaviorStackBlocks.InvalidBlock.BottomFailedFilter",
                     "bottom block failed it's filter");
    PrintCubeDebug("BehaviorStackBlocks.InvalidBlock.BottomFailedFilter.Debug", bottomObject);
    return false;
  }

  return true;
}

IBehavior::Status BehaviorStackBlocks::UpdateInternal(Robot& robot)
{
  if(_waitingForBlockToBeValid) {
    UpdateTargetBlocks(robot);
    if( AreBlocksAreStillValid(robot) ) {
      PRINT_NAMED_DEBUG("BehaviorStackBlocks.WaitForValid",
                        "Got valid blocks! resuming behavior");
      _waitingForBlockToBeValid = false;
      TransitionToPickingUpBlock(robot);
    }
  }

  IBehavior::Status ret = IBehavior::UpdateInternal(robot);

  // workaround for bugs that leave us stuck with a cube in our hands
  if( ret != Status::Running && robot.IsCarryingObject() && !_waitingForBlockToBeValid) {
    TransitionToWaitForBlocksToBeValid(robot);
    return Status::Running;
  }
  
  return ret;
}

void BehaviorStackBlocks::TransitionToPickingUpBlock(Robot& robot)
{
  DEBUG_SET_STATE(PickingUpBlock);
  // check that blocks are still good
  if( ! AreBlocksAreStillValid(robot) ) {
    // uh oh, blocks are no good, see if we can pick new ones
    UpdateTargetBlocks(robot);
    if( IsRunnable(robot) ) {
      // ok, found some new blocks, use those
      PRINT_NAMED_INFO("BehaviorStackBlocks.Picking.RestartWithNewBlocks",
                       "had to change blocks, re-starting behavior");
      // fall through to the function, which will now operate with the new blocks
    }
    else {
      TransitionToWaitForBlocksToBeValid(robot);
      return;
    }
  }

  // if we are already holding the block, skip
  if( robot.IsCarryingObject() && robot.GetCarryingObject() == _targetBlockTop ) {
    PRINT_NAMED_DEBUG("BehaviorStackBlocks.SkipPickup",
                      "Already holding top block, so no need to pick it up");
    TransitionToStackingBlock(robot);
  }

  //skips turning towards face if this action is streamlined
  const f32 angleTurnTowardsFace = _shouldStreamline ? 0 : kBSB_MaxTurnTowardsFaceBeforePickupAngle_deg;
  DriveToPickupObjectAction* action = new DriveToPickupObjectAction(robot, _targetBlockTop,
                                                                    false, 0, false,
                                                                    angleTurnTowardsFace,
                                                                    false);
  action->SetSayNameAnimationTrigger(AnimationTrigger::StackBlocksPreActionNamedFace);
  action->SetNoNameAnimationTrigger(AnimationTrigger::StackBlocksPreActionUnnamedFace);

  
  RetryWrapperAction::RetryCallback retryCallback = [this, action](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
  {
    animTrigger = AnimationTrigger::Count;
    
    // Don't turn towards face if retrying however this does still cause the head to move up
    action->SetMaxTurnTowardsFaceAngle(0);
    action->SetTiltTolerance(DEG_TO_RAD(180));

    if(completion.result == ActionResult::FAILURE_ABORT ||
       completion.result == ActionResult::FAILURE_RETRY)
    {
      switch(completion.completionInfo.Get_objectInteractionCompleted().result)
      {
        case ObjectInteractionResult::VISUAL_VERIFICATION_FAILED:
        {
          animTrigger = AnimationTrigger::StackBlocksRetry;
          
          // Use a different preAction pose if we failed because we couldn't see the block
          action->GetDriveToObjectAction()->SetGetPossiblePosesFunc([this, action](ActionableObject* object,
                                                                                   std::vector<Pose3d>& possiblePoses,
                                                                                   bool& alreadyInPosition)
                                                                    {
                                                                      return IBehavior::UseSecondClosestPreActionPose(action->GetDriveToObjectAction(), object, possiblePoses, alreadyInPosition);
                                                                    });
          break;
        }
          
        case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
        case ObjectInteractionResult::INCOMPLETE:
        {
          // Smaller reaction if we just didn't get to the pre-action pose
          animTrigger = AnimationTrigger::RollBlockRealign;
          break;
        }
          
        default:
          animTrigger = AnimationTrigger::StackBlocksRetry;
          break;
      } // switch(objectInteractionResult)
      return true;
    }

    return false;
  };
  
  RetryWrapperAction* retryWrapperAction = new RetryWrapperAction(robot, action, retryCallback, kBSB_MaxNumPickupRetries);
  
  StartActing(retryWrapperAction,
              [this,&robot](const ExternalInterface::RobotCompletedAction& msg)
              {
                if(msg.result == ActionResult::SUCCESS)
                {
                  TransitionToStackingBlock(robot);
                }
                else
                {
                  TransitionToWaitForBlocksToBeValid(robot);
                }
              });
    
  IncreaseScoreWhileActing( kBSB_ScoreIncreaseForAction );
}

void BehaviorStackBlocks::TransitionToStackingBlock(Robot& robot)
{
  DEBUG_SET_STATE(StackingBlock);
  // check that blocks are still good
  if( ! AreBlocksAreStillValid(robot) ) {
    // uh oh, blocks are no good, see if we can pick new ones
    UpdateTargetBlocks(robot);
    if( IsRunnable(robot) ) {
      // ok, found some new blocks, use those
      PRINT_NAMED_INFO("BehaviorStackBlocks.Stacking.RestartWithNewBlocks.",
                       "had to change blocks, re-starting behavior");
      TransitionToPickingUpBlock(robot);
    }
    else {
      TransitionToWaitForBlocksToBeValid(robot);
    }
    return;
  }
  
  // if we aren't carrying the top block, fail back to pick up
  if( ! robot.IsCarryingObject() ) {
    PRINT_NAMED_DEBUG("BehaviorStackBlocks.FailBackToPickup",
                      "wanted to stack, but we aren't carrying a block");
    TransitionToPickingUpBlock(robot);
  }
  else {
    StartActing(new DriveToPlaceOnObjectAction(robot, _targetBlockBottom),
                [this, &robot](ActionResult res) {
                  if( res == ActionResult::SUCCESS ) {
                    if(!_shouldStreamline){
                      TransitionToPlayingFinalAnim(robot);
                    }
                    BehaviorObjectiveAchieved(BehaviorObjective::StackedBlock);
                  }
                  else if( res == ActionResult::FAILURE_RETRY ) {
                    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::StackBlocksRetry),
                                &BehaviorStackBlocks::TransitionToStackingBlock);
                  }
                  else if( res == ActionResult::FAILURE_ABORT ) {
                    TransitionToWaitForBlocksToBeValid(robot);
                    // else we lost the block, but somehow still failed the placement action, so stop the behavior
                  }
                  // else end the behavior (other failure type)
                });
    IncreaseScoreWhileActing( kBSB_ScoreIncreaseForAction );
  }
}

void BehaviorStackBlocks::TransitionToWaitForBlocksToBeValid(Robot& robot)
{
  _waitingForBlockToBeValid = true;
  DEBUG_SET_STATE(WaitForBlocksToBeValid);
  
  // wait a bit to see if things settle and the cubes become valid (e.g. they were moving, so give them some
  // time to settle). If they become stable, Update will transition us out
  if (robot.IsCarryingObject())
  {
    CompoundActionSequential* action = new CompoundActionSequential(robot, {new DriveStraightAction(robot, -_distToBackupOnStackFailure_mm, DEFAULT_PATH_MOTION_PROFILE.speed_mmps), new PlaceObjectOnGroundAction(robot)});
    StartActing(action);
  }
}


void BehaviorStackBlocks::TransitionToPlayingFinalAnim(Robot& robot)
{
  DEBUG_SET_STATE(PlayingFinalAnim);
  
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::StackBlocksSuccess));
  IncreaseScoreWhileActing( kBSB_ScoreIncreaseForAction );
  
}

void BehaviorStackBlocks::ResetBehavior(const Robot& robot)
{
  _targetBlockTop.UnSet();
  _targetBlockBottom.UnSet();
}

void BehaviorStackBlocks::PrintCubeDebug(const char* event, const ObservableObject* obj) const
{
  const char* poseStateStr = "";
  switch(obj->GetPoseState()) {
    case PoseState::Known: poseStateStr = "known"; break;
    case PoseState::Unknown: poseStateStr = "unknown"; break;
    case PoseState::Dirty: poseStateStr = "dirty"; break;
  }
  
  PRINT_NAMED_DEBUG(event,
                    "block %d: blockUpright?%d CanPickUpObject%d CanStackOnTopOfObject?%d poseState=%s moving?%d restingFlat?%d",
                    obj->GetID().GetValue(),
                    obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS,
                    _robot.CanPickUpObject(*obj),
                    _robot.CanStackOnTopOfObject(*obj),
                    poseStateStr,
                    obj->IsMoving(),
                    obj->IsRestingFlat());
}

void BehaviorStackBlocks::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case EngineToGameTag::RobotOffTreadsStateChanged:
    {
      if(event.GetData().Get_RobotOffTreadsStateChanged().treadsState == OffTreadsState::OnTreads){
        ResetBehavior(robot);
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

}
}
