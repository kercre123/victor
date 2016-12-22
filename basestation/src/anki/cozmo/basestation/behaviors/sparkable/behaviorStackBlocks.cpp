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

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqNone.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(f32, kBSB_ScoreIncreaseForAction, "Behavior.StackBlocks", 0.8f);
CONSOLE_VAR(f32, kBSB_MaxTurnTowardsFaceBeforePickupAngle_deg, "Behavior.StackBlocks", 90.f);
CONSOLE_VAR(s32, kBSB_MaxNumPickupRetries, "Behavior.StackBlocks", 2);

static const char* const kStackInAnyOrientationKey = "stackInAnyOrientation";
static const float kWaitForValidTimeout_s = 0.4f;
static const float kTimeObjectInvalidAfterStackFailure_sec = 3.0f;
}

  
BehaviorStackBlocks::BehaviorStackBlocks(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _blockworldFilterForBottom( new BlockWorldFilter )
, _robot(robot)
{
  SetDefaultName("StackBlocks");
  
  _stackInAnyOrientation = config.get(kStackInAnyOrientationKey, false).asBool();

  // set up the filter we will use for finding blocks we care about
  _blockworldFilterForBottom->OnlyConsiderLatestUpdate(false);
  _blockworldFilterForBottom->SetFilterFcn( std::bind( &BehaviorStackBlocks::FilterBlocksForBottom,
                                                       this,
                                                       std::placeholders::_1) );
  
  SubscribeToTags({
    EngineToGameTag::RobotOffTreadsStateChanged
  });
}

bool BehaviorStackBlocks::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  // don't change blocks while we're running
  if( !IsRunning() ) {
    UpdateTargetBlocks(robot);
  }
  
  return _targetBlockBottom.IsSet() && _targetBlockTop.IsSet();
}

Result BehaviorStackBlocks::InitInternal(Robot& robot)
{
  if(robot.GetAIComponent().GetWhiteboard().HasTapIntent())
  {
    UpdateTargetBlocks(robot);
  }

  _waitForBlocksToBeValidUntilTime_s = -1.0f;
  TransitionToPickingUpBlock(robot);
  return Result::RESULT_OK;
}
  
void BehaviorStackBlocks::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorStackBlocks::StopInternalFromDoubleTap(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorStackBlocks::UpdateTargetBlocks(const Robot& robot) const
{
  // if we've got a cube in our lift, use that for top
  const ObjectID lastTopID = _targetBlockTop;
  _targetBlockTop.UnSet();
  if( robot.IsCarryingObject() ) {
    const ObservableObject* carriedObject = robot.GetBlockWorld().GetLocatedObjectByID( robot.GetCarryingObject() );

    if( nullptr != carriedObject ) {
      const bool forFreeplay = true;
      const bool isRollingUnlocked = robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::RollCube, forFreeplay);
      const bool upAxisOk = ! isRollingUnlocked ||
        carriedObject->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;

      if( upAxisOk || _stackInAnyOrientation ) {
        _targetBlockTop = carriedObject->GetID();
      }
    }
  }

  if( ! _targetBlockTop.IsSet() ) {
    const auto& whiteboard = robot.GetAIComponent().GetWhiteboard();
    const AIWhiteboard::ObjectUseIntention intention =
      _stackInAnyOrientation ?
      AIWhiteboard::ObjectUseIntention::PickUpAnyObject :
      AIWhiteboard::ObjectUseIntention::PickUpObjectWithAxisCheck;
    _targetBlockTop = whiteboard.GetBestObjectForAction(intention);
    _topBlockSetFromTapIntent = _robot.GetAIComponent().GetWhiteboard().HasTapIntent();
  }

  if( lastTopID.IsSet() && ! _targetBlockTop.IsSet() ) {
    const ObservableObject* lastTop = robot.GetBlockWorld().GetLocatedObjectByID(lastTopID);
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
      const ObservableObject* oldBottom = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockBottom);
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
  const bool forFreeplay = true;
  const bool isRollingUnlocked = _robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::RollCube, forFreeplay);
  const bool upAxisOk = ! isRollingUnlocked ||
    obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;
  
  return (obj->GetFamily() == ObjectFamily::LightCube &&
          !obj->IsPoseStateUnknown() &&
          (upAxisOk || _stackInAnyOrientation));
}

bool BehaviorStackBlocks::FilterBlocksForBottom(const ObservableObject* obj) const
{
  const bool hasFailedRecently = _robot.GetAIComponent().GetWhiteboard().
  DidFailToUse(obj->GetID(),
               AIWhiteboard::ObjectUseAction::StackOnObject,
               kTimeObjectInvalidAfterStackFailure_sec,
               obj->GetPose(),
               DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
               DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
  
  // top gets picked first, so can't pick top here
  bool ret = (obj->GetID() != _targetBlockTop &&
              FilterBlocksHelper(obj) &&
              !hasFailedRecently &&
              _robot.CanStackOnTopOfObject( *obj ));
  
  // If the top block wasn't set via a double tap and we have a tap intention then
  // the double tapped block should be the bottom block
  if(ret &&
     !_topBlockSetFromTapIntent &&
     _robot.GetAIComponent().GetWhiteboard().HasTapIntent())
  {
    using Intent = AIWhiteboard::ObjectUseIntention;
    Intent intent = _stackInAnyOrientation ? Intent::PickUpAnyObject : Intent::PickUpObjectWithAxisCheck;
    
    const ObjectID id = _robot.GetAIComponent().GetWhiteboard().GetBestObjectForAction(intent);
    ret = (id == obj->GetID());
  }
  
  return ret;
}

bool BehaviorStackBlocks::AreBlocksStillValid(const Robot& robot)
{
  if( !_targetBlockTop.IsSet() || !_targetBlockBottom.IsSet() ) {

    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const bool waitingForValid = currTime_s <= _waitForBlocksToBeValidUntilTime_s;
    if( !waitingForValid ) {
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
    const ObservableObject* topObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockTop);
    if( topObject == nullptr ) {
      PRINT_NAMED_INFO("BehaviorStackBlocks.InvalidBlock.BlockDeleted",
                       "target block %d has no pointer in blockworld",
                       _targetBlockTop.GetValue());
      _targetBlockTop.UnSet();
      return false;
    }


    const auto& whiteboard = robot.GetAIComponent().GetWhiteboard();
    const AIWhiteboard::ObjectUseIntention intention =
      _stackInAnyOrientation ?
        AIWhiteboard::ObjectUseIntention::PickUpAnyObject :
        AIWhiteboard::ObjectUseIntention::PickUpObjectWithAxisCheck;
    
    if( ! whiteboard.IsObjectValidForAction(intention, _targetBlockTop ) ) {
      PRINT_NAMED_INFO("BehaviorStackBlocks.InvalidBlock.TopFailedFilter",
                       "top block failed it's filter");
      _targetBlockTop.UnSet();
      PrintCubeDebug("BehaviorStackBlocks.InvalidBlock.TopFailedFilter.Debug", topObject);
      return false;
    }
  }
  
  const ObservableObject* bottomObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockBottom);
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
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool waitingForValid = currTime_s <= _waitForBlocksToBeValidUntilTime_s;
  if( waitingForValid ) {
    UpdateTargetBlocks(robot);
    if( AreBlocksStillValid(robot) ) {
      PRINT_NAMED_DEBUG("BehaviorStackBlocks.WaitForValid",
                        "Got valid blocks! resuming behavior");
      _waitForBlocksToBeValidUntilTime_s = -1.0f;
      TransitionToPickingUpBlock(robot);
    }
  }
  
  // Check to see if better bottom block identified for stacking on
  // New bottom must be closer to cozmo and currently visible while the old target base is not
  if(robot.IsCarryingObject()){
    const ObservableObject* newBottomObject = robot.GetBlockWorld().FindObjectClosestTo(robot.GetPose(),
                                                                                     *_blockworldFilterForBottom);
    const ObservableObject* currentTarget = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockBottom);
    
    if( nullptr != newBottomObject && newBottomObject->GetID() != _targetBlockBottom) {
      const bool currentTargetSeenThisFrame = currentTarget != nullptr &&
                         currentTarget->GetLastObservedTime() ==robot.GetLastImageTimeStamp();
      const bool newBottomSeenThisFrame = newBottomObject->GetLastObservedTime() == robot.GetLastImageTimeStamp();

      if(!currentTargetSeenThisFrame && newBottomSeenThisFrame){
        StopWithoutImmediateRepetitionPenalty();
        return Status::Complete;
      }
    }
  }

  IBehavior::Status ret = IBehavior::UpdateInternal(robot);
  
  return ret;
}

void BehaviorStackBlocks::TransitionToPickingUpBlock(Robot& robot)
{
  DEBUG_SET_STATE(PickingUpBlock);
  // check that blocks are still good
  if( ! AreBlocksStillValid(robot) ) {
    // uh oh, blocks are no good, see if we can pick new ones
    UpdateTargetBlocks(robot);
    BehaviorPreReqNone preReqData;
    if( IsRunnableScored(preReqData) ) {
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
    return;
  }

  //skips turning towards face if this action is streamlined
  const f32 angleTurnTowardsFace = _shouldStreamline ? 0 : kBSB_MaxTurnTowardsFaceBeforePickupAngle_deg;
  DriveToPickupObjectAction* action = new DriveToPickupObjectAction(robot, _targetBlockTop,
                                                                    false, 0, false,
                                                                    angleTurnTowardsFace,
                                                                    false);
  action->SetSayNameAnimationTrigger(AnimationTrigger::StackBlocksPreActionNamedFace);
  action->SetNoNameAnimationTrigger(AnimationTrigger::StackBlocksPreActionUnnamedFace);
  
  RetryWrapperAction::RetryCallback retryCallback =
    [this, action, &robot](const ExternalInterface::RobotCompletedAction& completion,
                           const u8 retryCount,
                           AnimationTrigger& animTrigger)
  {
    animTrigger = AnimationTrigger::Count;
    
    const auto& whiteboard = robot.GetAIComponent().GetWhiteboard();
    const AIWhiteboard::ObjectUseIntention intention =
      _stackInAnyOrientation ?
        AIWhiteboard::ObjectUseIntention::PickUpAnyObject :
        AIWhiteboard::ObjectUseIntention::PickUpObjectWithAxisCheck;

    const bool blockStillValid = whiteboard.IsObjectValidForAction(intention, _targetBlockTop);
    if( ! blockStillValid ) {
      return false;
    }
    
    // Don't turn towards face if retrying
    action->DontTurnTowardsFace();
    
    animTrigger = AnimationTrigger::StackBlocksRetry;
    
    if(completion.result == ActionResult::VISUAL_OBSERVATION_FAILED)
    {
      // Use a different preAction pose if we failed because we couldn't see the block
      action->GetDriveToObjectAction()->SetGetPossiblePosesFunc(
        [this, action](ActionableObject* object,
                       std::vector<Pose3d>& possiblePoses,
                       bool& alreadyInPosition)
        {
          return IBehavior::UseSecondClosestPreActionPose(action->GetDriveToObjectAction(),
                                                          object,
                                                          possiblePoses,
                                                          alreadyInPosition);
        });
      return true;
    }
    
    const bool isRetryResult = (IActionRunner::GetActionResultCategory(completion.result) == ActionResultCategory::RETRY);
    
    if(isRetryResult &&
       completion.result != ActionResult::LAST_PICK_AND_PLACE_FAILED &&
       completion.result != ActionResult::NOT_CARRYING_OBJECT_RETRY)
    {
        // Smaller reaction if we didn't fail docking
        // This is the intended animation trigger for now - don't change without consulting Mooly
        animTrigger = AnimationTrigger::RollBlockRealign;
    }

    // we want to retry if the action says we should
    return isRetryResult;
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
                  // mark the block as inaccessible if we've retried the appropriate number of times
                  const ObservableObject* failedObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockTop);
                  if(failedObject){
                    robot.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject,
                                                                              AIWhiteboard::ObjectUseAction::PickUpObject);
                  }
                 
                  // Play failure animation
                  if (msg.result == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_MOVING || msg.result == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_NOT_MOVING) {
                    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::StackBlocksRetry), &BehaviorStackBlocks::TransitionToWaitForBlocksToBeValid);
                  } else {
                    TransitionToWaitForBlocksToBeValid(robot);
                  }
                }
              });
    
  IncreaseScoreWhileActing( kBSB_ScoreIncreaseForAction );
}

void BehaviorStackBlocks::TransitionToStackingBlock(Robot& robot)
{
  DEBUG_SET_STATE(StackingBlock);
  // check that blocks are still good
  if( ! AreBlocksStillValid(robot) ) {
    // uh oh, blocks are no good, see if we can pick new ones
    UpdateTargetBlocks(robot);
    BehaviorPreReqNone preReqData;
    if( IsRunnableScored(preReqData) ) {
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
                [this, &robot](const ExternalInterface::RobotCompletedAction& completion) {
                  ActionResultCategory resCat = IActionRunner::GetActionResultCategory(completion.result);
                  
                  if( resCat == ActionResultCategory::SUCCESS ) {
                    if(!_shouldStreamline){
                      TransitionToPlayingFinalAnim(robot);
                    }
                    BehaviorObjectiveAchieved(BehaviorObjective::StackedBlock);
                  }
                  else if( resCat == ActionResultCategory::RETRY ) {
                    StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::StackBlocksRetry),
                                &BehaviorStackBlocks::TransitionToStackingBlock);
                  }
                  else if( resCat == ActionResultCategory::ABORT ) {
                    // mark the block as failed to stack on
                    const ObservableObject* failedObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockBottom);
                    if(failedObject){
                      robot.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject,
                                                                                AIWhiteboard::ObjectUseAction::StackOnObject);
                    }
                    
                    if( completion.result == ActionResult::STILL_CARRYING_OBJECT ) {
                      // robot thinks it should have stacked, but also thinks it is still carrying. This
                      // likely means it never was carrying in the first place, so go ahead and do a "put
                      // down" action, which will likely just verify that we don't have the cube in our lift
                      // anymore (someone removed it or we never had it in the first place)

                      CompoundActionSequential* placeAction = new CompoundActionSequential(robot, {
                          new DriveStraightAction(robot,
                                                  -_distToBackupOnStackFailure_mm,
                                                  DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
                            new PlaceObjectOnGroundAction(robot)});
                      StartActing(placeAction);
                    }
                    else {
                      // some other abort failure, give the cubes a little time to settle and then bail out of
                      // the behavior
                      TransitionToWaitForBlocksToBeValid(robot);
                    }
                  }
                  // else end the behavior (other failure type)
                });
    IncreaseScoreWhileActing( kBSB_ScoreIncreaseForAction );
  }
}

void BehaviorStackBlocks::TransitionToWaitForBlocksToBeValid(Robot& robot)
{
  _waitForBlocksToBeValidUntilTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kWaitForValidTimeout_s;
  DEBUG_SET_STATE(WaitForBlocksToBeValid);
  
  // wait a bit to see if things settle and the cubes become valid (e.g. they were moving, so give them some
  // time to settle). If they become stable, Update will transition us out
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
  _topBlockSetFromTapIntent = false;
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
