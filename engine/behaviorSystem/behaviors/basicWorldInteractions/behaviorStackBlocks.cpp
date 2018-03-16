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

#include "engine/behaviorSystem/behaviors/basicWorldInteractions/behaviorStackBlocks.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(f32, kBSB_ScoreIncreaseForAction, "Behavior.StackBlocks", 0.8f);
CONSOLE_VAR(f32, kBSB_MaxTurnTowardsFaceBeforePickupAngle_deg, "Behavior.StackBlocks", 90.f);
CONSOLE_VAR(bool, kCanHiccupWhileStacking, "Hiccups", true);

static const char* const kStackInAnyOrientationKey = "stackInAnyOrientation";
  static const f32   kDistToBackupOnStackFailure_mm = 40;

static constexpr ReactionTriggerHelpers::FullReactionArray kHiccupDisableTriggers = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStackBlocks::BehaviorStackBlocks(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _robot(robot)
, _stackInAnyOrientation(false)
, _hasBottomTargetSwitched(false)
{
  _stackInAnyOrientation = config.get(kStackInAnyOrientationKey, false).asBool();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorStackBlocks::IsRunnableInternal(const Robot& robot) const
{
  UpdateTargetBlocks(robot);
  return _targetBlockBottom.IsSet() && _targetBlockTop.IsSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorStackBlocks::InitInternal(Robot& robot)
{
  if(robot.GetCarryingComponent().GetCarryingObject() == _targetBlockTop){
    TransitionToStackingBlock(robot);
  }else{
    TransitionToPickingUpBlock(robot);
  }
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorStackBlocks::CanUseNonUprightBlocks(const Robot& robot) const
{
  const bool forFreeplay = true;
  const bool isRollingUnlocked = robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::RollCube, forFreeplay);
  return isRollingUnlocked || _stackInAnyOrientation;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::UpdateTargetBlocks(const Robot& robot) const
{
  auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();
  
  if(CanUseNonUprightBlocks(robot)){
    _targetBlockTop = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackTopObjectNoAxisCheck);
    _targetBlockBottom = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackBottomObjectNoAxisCheck);
  }else{
    _targetBlockTop = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackTopObjectAxisCheck);
    _targetBlockBottom = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackBottomObjectAxisCheck);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorStackBlocks::UpdateInternal(Robot& robot)
{
  auto topBlockIntention  = ObjectInteractionIntention::StackTopObjectAxisCheck;
  auto bottomBlockIntention  = ObjectInteractionIntention::StackBottomObjectAxisCheck;

  if(CanUseNonUprightBlocks(robot)){
    topBlockIntention  = ObjectInteractionIntention::StackTopObjectNoAxisCheck;
    bottomBlockIntention  = ObjectInteractionIntention::StackBottomObjectNoAxisCheck;
  }
  
  auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();

  // Verify that blocks are still valid
  auto validTopObjs = objInfoCache.GetValidObjectsForIntention(topBlockIntention);
  const bool topValid = validTopObjs.find(_targetBlockTop) != validTopObjs.end();
  
  // Bottom block validity doesn't matter if we haven't picked up the top block yet
  // this prevents stopping when the top block has just been placed
  auto validBottomObjs = objInfoCache.GetValidObjectsForIntention(bottomBlockIntention);
  const bool bottomValid = validBottomObjs.find(_targetBlockBottom) != validBottomObjs.end();
  
  if(!(topValid && bottomValid)){
    // if the bottom block is the one that's not valid, check if that's because
    // the top block is now on top of it - in which case don't cancel the behavior
    bool shouldStop = true;
    if(!bottomValid){
      const ObservableObject* bottomObj = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockBottom);
      const ObservableObject* topObj    = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockTop);
      
      if((bottomObj != nullptr)  &&
         (topObj != nullptr)){
        auto objOnTop = robot.GetBlockWorld().FindLocatedObjectOnTopOf(*bottomObj,
                                                                       STACKED_HEIGHT_TOL_MM);
        if(objOnTop == topObj){
          shouldStop = false;
        }
      }
    }
    
    if(shouldStop){
      PRINT_CH_DEBUG("Behaviors",
                     "BehaviorStackBlocks.UpdateInternal.BlocksInvalid",
                     "Stopping due to invalid blocks topBlockValid:%d bottomBlockValid:%d",
                     topValid,
                     bottomValid);
      StopWithoutImmediateRepetitionPenalty();
      return BehaviorStatus::Complete;
    }
  }
  
  // Check to see if a better base block has become available for stacking on top of
  // only do this once per behavior run so that we don't throttle between blocks
  // when aligning - this is to catch cases where kids shove blocks to cozmo
  if((_behaviorState == State::StackingBlock)  &&
     !_hasBottomTargetSwitched){
    
    
    auto bestBottom = GetClosestValidBottom(robot, bottomBlockIntention);
    if(bestBottom != _targetBlockBottom){
      StopActing(false);
      _targetBlockBottom = bestBottom;
      _hasBottomTargetSwitched = true;
      TransitionToStackingBlock(robot);
    }
  }
  

  IBehavior::Status ret = IBehavior::UpdateInternal(robot);
  
  return ret;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToPickingUpBlock(Robot& robot)
{
  SET_STATE(PickingUpBlock);

  
  PickupBlockParamaters params;
  params.maxTurnTowardsFaceAngle_rad = ShouldStreamline() ? 0 : kBSB_MaxTurnTowardsFaceBeforePickupAngle_deg;
  HelperHandle pickupHelper = GetBehaviorHelperFactory().
                                      CreatePickupBlockHelper(robot, *this,
                                                              _targetBlockTop, params);
  
  SmartDelegateToHelper(robot, pickupHelper, &BehaviorStackBlocks::TransitionToStackingBlock);
  IncreaseScoreWhileActing( kBSB_ScoreIncreaseForAction );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToStackingBlock(Robot& robot)
{
  SET_STATE(StackingBlock);
  
  // if we aren't carrying the top block, fail back to pick up
  const bool holdingTopBlock = robot.GetCarryingComponent().IsCarryingObject() &&
                               robot.GetCarryingComponent().GetCarryingObject() == _targetBlockTop;
  if( ! holdingTopBlock ) {
    PRINT_NAMED_DEBUG("BehaviorStackBlocks.FailBackToPickup",
                      "wanted to stack, but we aren't carrying a block");
    TransitionToPickingUpBlock(robot);
    return;
  }
  // Disable hiccup so it can't interrupt the stack
  if(!kCanHiccupWhileStacking)
  {
    SmartDisableReactionsWithLock(GetIDStr(), kHiccupDisableTriggers);
  }
  
  const bool placingOnGround = false;
  HelperHandle placeHelper = GetBehaviorHelperFactory().
                      CreatePlaceRelObjectHelper(robot, *this,
                                                 _targetBlockBottom,
                                                 placingOnGround);

  SmartDelegateToHelper(robot, placeHelper,
                        &BehaviorStackBlocks::TransitionToPlayingFinalAnim,
                        &BehaviorStackBlocks::TransitionToFailedToStack);
  IncreaseScoreWhileActing( kBSB_ScoreIncreaseForAction );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToPlayingFinalAnim(Robot& robot)
{
  SET_STATE(PlayingFinalAnim);
  
  BehaviorObjectiveAchieved(BehaviorObjective::StackedBlock);
  if(!ShouldStreamline()){
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::StackBlocksSuccess));
    IncreaseScoreWhileActing( kBSB_ScoreIncreaseForAction );
  }
  NeedActionCompleted();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToFailedToStack(Robot& robot)
{
  auto retryIfPossible = [this](Robot& robot){
    UpdateTargetBlocks(robot);
    if(_targetBlockTop.IsSet() && _targetBlockBottom.IsSet()){
      TransitionToPickingUpBlock(robot);
    }
  };
  
  // If cozmo thinks he's still carrying the cube, try placing it on the ground to
  // see if it's really there - then see if we can try again
  if(robot.GetCarryingComponent().IsCarryingObject()){
    CompoundActionSequential* placeAction = new CompoundActionSequential(robot, {
      new DriveStraightAction(robot,
                              -kDistToBackupOnStackFailure_mm,
                              DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
      new PlaceObjectOnGroundAction(robot)});
    StartActing(placeAction, retryIfPossible);
  }else{
    retryIfPossible(robot);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorStackBlocks::GetClosestValidBottom(Robot& robot, ObjectInteractionIntention bottomIntention) const
{
  DEV_ASSERT(_targetBlockBottom.IsSet(),
             "BehaviorStackBlocks.GetClosestValidBottom.TargetBlockNotValid");
  
  auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();

  ObjectID bestBottom = _targetBlockBottom;
  const ObservableObject* currentTarget =  robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockBottom);
  f32 distToClosestBottom_mm_sq;
  
  if((currentTarget != nullptr) &&
     ComputeDistanceSQBetween(robot.GetPose(), currentTarget->GetPose(), distToClosestBottom_mm_sq)){
    auto validObjs = objInfoCache.GetValidObjectsForIntention(bottomIntention);
    for(const auto& objID : validObjs){
      const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(objID);
      f32 distanceToValid;
      if((obj != nullptr) &&
         ComputeDistanceSQBetween(robot.GetPose(), obj->GetPose(), distanceToValid)){
        if(distanceToValid < distToClosestBottom_mm_sq){
          bestBottom = objID;
          distToClosestBottom_mm_sq = distanceToValid;
        }
      }
    }
  }
  
  return bestBottom;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::ResetBehavior(const Robot& robot)
{
  _hasBottomTargetSwitched = false;
  _targetBlockTop.UnSet();
  _targetBlockBottom.UnSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::SetState_internal(State state, const std::string& stateName)
{
  _behaviorState = state;
  SetDebugStateName(stateName);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::PrintCubeDebug(const char* event, const ObservableObject* obj) const
{
  // this should be helper so that it can be reused
  const char* poseStateStr = "";
  switch(obj->GetPoseState()) {
    case PoseState::Known: poseStateStr = "known"; break;
    case PoseState::Dirty: poseStateStr = "dirty"; break;
    case PoseState::Invalid: poseStateStr = "invalid"; break;
  }
  
  PRINT_NAMED_DEBUG(event,
                    "block %d: blockUpright?%d CanPickUpObject%d CanStackOnTopOfObject?%d poseState=%s moving?%d restingFlat?%d",
                    obj->GetID().GetValue(),
                    obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS,
                    _robot.GetDockingComponent().CanPickUpObject(*obj),
                    _robot.GetDockingComponent().CanStackOnTopOfObject(*obj),
                    poseStateStr,
                    obj->IsMoving(),
                    obj->IsRestingFlat());
}


} // namespace Cozmo
} // namespace Anki
