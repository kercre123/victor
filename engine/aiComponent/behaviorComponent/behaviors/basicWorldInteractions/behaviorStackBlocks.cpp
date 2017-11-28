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

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorStackBlocks.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
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

static const char* const kStackInAnyOrientationKey = "stackInAnyOrientation";
  static const f32   kDistToBackupOnStackFailure_mm = 40;
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStackBlocks::BehaviorStackBlocks(const Json::Value& config)
: ICozmoBehavior(config)
, _stackInAnyOrientation(false)
, _hasBottomTargetSwitched(false)
{
  _stackInAnyOrientation = config.get(kStackInAnyOrientationKey, false).asBool();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorStackBlocks::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  UpdateTargetBlocks(behaviorExternalInterface);
  return _targetBlockBottom.IsSet() && _targetBlockTop.IsSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorStackBlocks::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  
  if(robot.GetCarryingComponent().GetCarryingObject() == _targetBlockTop){
    TransitionToStackingBlock(behaviorExternalInterface);
  }else{
    TransitionToPickingUpBlock(behaviorExternalInterface);
  }
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  ResetBehavior();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorStackBlocks::CanUseNonUprightBlocks(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool forFreeplay = true;
  bool isRollingUnlocked = true;
  
  if(behaviorExternalInterface.HasProgressionUnlockComponent()){
    auto& progressionUnlockComp = behaviorExternalInterface.GetProgressionUnlockComponent();
    isRollingUnlocked = progressionUnlockComp.IsUnlocked(UnlockId::RollCube, forFreeplay);
  }
  
  return isRollingUnlocked || _stackInAnyOrientation;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::UpdateTargetBlocks(BehaviorExternalInterface& behaviorExternalInterface) const
{
  auto& objInfoCache = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache();
  
  if(CanUseNonUprightBlocks(behaviorExternalInterface)){
    _targetBlockTop = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackTopObjectNoAxisCheck);
    _targetBlockBottom = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackBottomObjectNoAxisCheck);
  }else{
    _targetBlockTop = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackTopObjectAxisCheck);
    _targetBlockBottom = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackBottomObjectAxisCheck);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorStackBlocks::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto topBlockIntention  = ObjectInteractionIntention::StackTopObjectAxisCheck;
  auto bottomBlockIntention  = ObjectInteractionIntention::StackBottomObjectAxisCheck;

  if(CanUseNonUprightBlocks(behaviorExternalInterface)){
    topBlockIntention  = ObjectInteractionIntention::StackTopObjectNoAxisCheck;
    bottomBlockIntention  = ObjectInteractionIntention::StackBottomObjectNoAxisCheck;
  }
  
  auto& objInfoCache = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache();

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
      const ObservableObject* bottomObj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetBlockBottom);
      const ObservableObject* topObj    = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetBlockTop);
      
      if((bottomObj != nullptr)  &&
         (topObj != nullptr)){
        auto objOnTop = behaviorExternalInterface.GetBlockWorld().FindLocatedObjectOnTopOf(*bottomObj,
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
      CancelSelf();
      return BehaviorStatus::Running;
    }
  }
  
  // Check to see if a better base block has become available for stacking on top of
  // only do this once per behavior run so that we don't throttle between blocks
  // when aligning - this is to catch cases where kids shove blocks to cozmo
  if((_behaviorState == State::StackingBlock)  &&
     !_hasBottomTargetSwitched){
    
    
    auto bestBottom = GetClosestValidBottom(behaviorExternalInterface, bottomBlockIntention);
    if(bestBottom != _targetBlockBottom){
      CancelDelegates(false);
      _targetBlockBottom = bestBottom;
      _hasBottomTargetSwitched = true;
      TransitionToStackingBlock(behaviorExternalInterface);
    }
  }
  

  ICozmoBehavior::Status ret = ICozmoBehavior::UpdateInternal_WhileRunning(behaviorExternalInterface);
  
  return ret;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToPickingUpBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(PickingUpBlock);

  
  PickupBlockParamaters params;
  params.maxTurnTowardsFaceAngle_rad = ShouldStreamline() ? 0 : kBSB_MaxTurnTowardsFaceBeforePickupAngle_deg;
  HelperHandle pickupHelper = GetBehaviorHelperFactory().
                                      CreatePickupBlockHelper(behaviorExternalInterface, *this,
                                                              _targetBlockTop, params);
  
  SmartDelegateToHelper(behaviorExternalInterface, pickupHelper, &BehaviorStackBlocks::TransitionToStackingBlock);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToStackingBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(StackingBlock);
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  // if we aren't carrying the top block, fail back to pick up
  const bool holdingTopBlock = robot.GetCarryingComponent().IsCarryingObject() &&
                               robot.GetCarryingComponent().GetCarryingObject() == _targetBlockTop;
  if( ! holdingTopBlock ) {
    PRINT_NAMED_DEBUG("BehaviorStackBlocks.FailBackToPickup",
                      "wanted to stack, but we aren't carrying a block");
    TransitionToPickingUpBlock(behaviorExternalInterface);
    return;
  }
  
  const bool placingOnGround = false;
  HelperHandle placeHelper = GetBehaviorHelperFactory().
                      CreatePlaceRelObjectHelper(behaviorExternalInterface, *this,
                                                 _targetBlockBottom,
                                                 placingOnGround);

  SmartDelegateToHelper(behaviorExternalInterface, placeHelper,
                        &BehaviorStackBlocks::TransitionToPlayingFinalAnim,
                        &BehaviorStackBlocks::TransitionToFailedToStack);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToPlayingFinalAnim(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(PlayingFinalAnim);
  
  BehaviorObjectiveAchieved(BehaviorObjective::StackedBlock);
  if(!ShouldStreamline()){
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::StackBlocksSuccess));
  }
  NeedActionCompleted();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToFailedToStack(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  auto retryIfPossible = [this](BehaviorExternalInterface& behaviorExternalInterface){
    UpdateTargetBlocks(behaviorExternalInterface);
    if(_targetBlockTop.IsSet() && _targetBlockBottom.IsSet()){
      TransitionToPickingUpBlock(behaviorExternalInterface);
    }
  };
  
  // If cozmo thinks he's still carrying the cube, try placing it on the ground to
  // see if it's really there - then see if we can try again
  if(robot.GetCarryingComponent().IsCarryingObject()){
    CompoundActionSequential* placeAction = new CompoundActionSequential({
      new DriveStraightAction(-kDistToBackupOnStackFailure_mm,
                              DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
      new PlaceObjectOnGroundAction()});
    DelegateIfInControl(placeAction, retryIfPossible);
  }else{
    retryIfPossible(behaviorExternalInterface);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorStackBlocks::GetClosestValidBottom(BehaviorExternalInterface& behaviorExternalInterface, ObjectInteractionIntention bottomIntention) const
{
  DEV_ASSERT(_targetBlockBottom.IsSet(),
             "BehaviorStackBlocks.GetClosestValidBottom.TargetBlockNotValid");
  
  auto& objInfoCache = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache();

  ObjectID bestBottom = _targetBlockBottom;
  const ObservableObject* currentTarget =  behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetBlockBottom);
  f32 distToClosestBottom_mm_sq;
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  if((currentTarget != nullptr) &&
     ComputeDistanceSQBetween(robot.GetPose(), currentTarget->GetPose(), distToClosestBottom_mm_sq)){
    auto validObjs = objInfoCache.GetValidObjectsForIntention(bottomIntention);
    for(const auto& objID : validObjs){
      const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(objID);
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
void BehaviorStackBlocks::ResetBehavior()
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
void BehaviorStackBlocks::PrintCubeDebug(BehaviorExternalInterface& behaviorExternalInterface,
                                         const char* event, const ObservableObject* obj) const
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
                    behaviorExternalInterface.GetRobot().GetDockingComponent().CanPickUpObject(*obj),
                    behaviorExternalInterface.GetRobot().GetDockingComponent().CanStackOnTopOfObject(*obj),
                    poseStateStr,
                    obj->IsMoving(),
                    obj->IsRestingFlat());
}


} // namespace Cozmo
} // namespace Anki
