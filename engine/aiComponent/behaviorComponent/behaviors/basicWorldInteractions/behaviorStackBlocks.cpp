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

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/progressionUnlockComponent.h"

#include "clad/types/actionResults.h"
#include "coretech/vision/engine/observableObject.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(f32, kBSB_ScoreIncreaseForAction, "Behavior.StackBlocks", 0.8f);
CONSOLE_VAR(f32, kBSB_MaxTurnTowardsFaceBeforePickupAngle_deg, "Behavior.StackBlocks", 90.f);

static const char* const kStackInAnyOrientationKey = "stackInAnyOrientation";
static const char* const kPickupBehaviorIDKey      = "pickupBehaviorID";
static const char* const kDefaultPickupBehaviorID  = "PickupCube";
static const char* const kRetryPlaceCountKey       = "numPlaceRetries";
static const f32   kDistToBackupOnStackFailure_mm  = 40;
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStackBlocks::BehaviorStackBlocks(const Json::Value& config)
: ICozmoBehavior(config)
{
  _iConfig.stackInAnyOrientation = config.get(kStackInAnyOrientationKey, false).asBool();
  
  {
    std::string behaivorIDStr = kDefaultPickupBehaviorID;
    JsonTools::GetValueOptional(config, kPickupBehaviorIDKey, behaivorIDStr);
    _iConfig.pickupID = BehaviorIDFromString(behaivorIDStr);
  }

  if(config.isMember(kRetryPlaceCountKey)){
    _iConfig.placeRetryCount = JsonTools::ParseUint8(config, kRetryPlaceCountKey, 
                                                     "BehaviorStackBlocks.Constructor.RetryCountIssue");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::InitBehavior()
{
  auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast<BehaviorPickUpCube>(_iConfig.pickupID,
                                                     BEHAVIOR_CLASS(PickUpCube),
                                                     _iConfig.pickupBehavior);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::GetAllDelegates(std::set<IBehavior*>& delegates) const 
{ 
  delegates.insert(_iConfig.pickupBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorStackBlocks::WantsToBeActivatedBehavior() const
{
  ObjectID bottomBlock;
  ObjectID topBlock;
  CalculateTargetBlocks(bottomBlock, topBlock);
  return bottomBlock.IsSet() && topBlock.IsSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  CalculateTargetBlocks(_dVars.targetBlockBottom, _dVars.targetBlockTop);

  const auto& robotInfo = GetBEI().GetRobotInfo();
  if(robotInfo.GetCarryingComponent().GetCarryingObject() == _dVars.targetBlockTop){
    TransitionToStackingBlock();
  }else{
    TransitionToPickingUpBlock();
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorStackBlocks::CanUseNonUprightBlocks() const
{
  const bool forFreeplay = true;
  bool isRollingUnlocked = true;
  
  if(GetBEI().HasProgressionUnlockComponent()){
    auto& progressionUnlockComp = GetBEI().GetProgressionUnlockComponent();
    isRollingUnlocked = progressionUnlockComp.IsUnlocked(UnlockId::RollCube, forFreeplay);
  }
  
  return isRollingUnlocked || _iConfig.stackInAnyOrientation;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::CalculateTargetBlocks(ObjectID& bottomBlock, ObjectID& topBlock) const
{
  auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();
  
  if(CanUseNonUprightBlocks()){
    topBlock = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackTopObjectNoAxisCheck);
    bottomBlock = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackBottomObjectNoAxisCheck);
  }else{
    topBlock = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackTopObjectAxisCheck);
    bottomBlock = objInfoCache.GetBestObjectForIntention(ObjectInteractionIntention::StackBottomObjectAxisCheck);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  auto topBlockIntention  = ObjectInteractionIntention::StackTopObjectAxisCheck;
  auto bottomBlockIntention  = ObjectInteractionIntention::StackBottomObjectAxisCheck;

  if(CanUseNonUprightBlocks()){
    topBlockIntention  = ObjectInteractionIntention::StackTopObjectNoAxisCheck;
    bottomBlockIntention  = ObjectInteractionIntention::StackBottomObjectNoAxisCheck;
  }
  
  auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();

  // Verify that blocks are still valid
  auto validTopObjs = objInfoCache.GetValidObjectsForIntention(topBlockIntention);
  const bool topValid = validTopObjs.find(_dVars.targetBlockTop) != validTopObjs.end();
  
  // Bottom block validity doesn't matter if we haven't picked up the top block yet
  // this prevents stopping when the top block has just been placed
  auto validBottomObjs = objInfoCache.GetValidObjectsForIntention(bottomBlockIntention);
  const bool bottomValid = validBottomObjs.find(_dVars.targetBlockBottom) != validBottomObjs.end();
  
  if(!(topValid && bottomValid)){
    // if the bottom block is the one that's not valid, check if that's because
    // the top block is now on top of it - in which case don't cancel the behavior
    bool shouldStop = true;
    if(!bottomValid){
      const ObservableObject* bottomObj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetBlockBottom);
      const ObservableObject* topObj    = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetBlockTop);
      
      if((bottomObj != nullptr)  &&
         (topObj != nullptr)){
        auto objOnTop = GetBEI().GetBlockWorld().FindLocatedObjectOnTopOf(*bottomObj,
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
      return;
    }
  }

  // Attempt to transition to stacking - will fall back to another pickup if necessary
  if(_dVars.behaviorState == State::PickingUpBlock &&
     !IsControlDelegated()){
    TransitionToStackingBlock();
  }
  
  // Check to see if a better base block has become available for stacking on top of
  // only do this once per behavior run so that we don't throttle between blocks
  // when aligning - this is to catch cases where kids shove blocks to cozmo
  if((_dVars.behaviorState == State::StackingBlock)  &&
     !_dVars.hasBottomTargetSwitched){
    
    
    auto bestBottom = GetClosestValidBottom(bottomBlockIntention);
    if(bestBottom != _dVars.targetBlockBottom){
      CancelDelegates(false);
      _dVars.targetBlockBottom = bestBottom;
      _dVars.hasBottomTargetSwitched = true;
      TransitionToStackingBlock();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToPickingUpBlock()
{
  SET_STATE(PickingUpBlock);
  _iConfig.pickupBehavior->SetTargetID(_dVars.targetBlockTop);
  DelegateIfInControl(_iConfig.pickupBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToStackingBlock()
{
  SET_STATE(StackingBlock);
  
  const auto& robotInfo = GetBEI().GetRobotInfo();

  // if we aren't carrying the top block, fail back to pick up
  const bool holdingTopBlock = robotInfo.GetCarryingComponent().IsCarryingObject() &&
                               robotInfo.GetCarryingComponent().GetCarryingObject() == _dVars.targetBlockTop;
  if( ! holdingTopBlock ) {
    PRINT_NAMED_DEBUG("BehaviorStackBlocks.FailBackToPickup",
                      "wanted to stack, but we aren't carrying a block");
    TransitionToPickingUpBlock();
    return;
  }

  const bool placingOnGround = false;
  DelegateIfInControl(new PlaceRelObjectAction(_dVars.targetBlockBottom, placingOnGround),
                      [this](ActionResult result){
                        if(result == ActionResult::SUCCESS){
                          TransitionToPlayingFinalAnim();
                        }else if((IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY) &&
                                  (_dVars.placeRetryCount < _iConfig.placeRetryCount)){
                          _dVars.placeRetryCount++;
                          TransitionToStackingBlock();
                        }else{
                          auto& blockWorld = GetBEI().GetBlockWorld();
                          const ObservableObject* bottomObj = blockWorld.GetLocatedObjectByID(_dVars.targetBlockBottom);
                          if(bottomObj != nullptr){
                            auto& whiteboard = GetBEI().GetAIComponent().GetWhiteboard();	
                            whiteboard.SetFailedToUse(*bottomObj,	
                                                      AIWhiteboard::ObjectActionFailure::StackOnObject);
                          }
                        }        
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToPlayingFinalAnim()
{
  SET_STATE(PlayingFinalAnim);
  
  BehaviorObjectiveAchieved(BehaviorObjective::StackedBlock);
  if(!ShouldStreamline()){
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::StackBlocksSuccess));
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStackBlocks::TransitionToFailedToStack()
{
  auto retryIfPossible = [this](){
    CalculateTargetBlocks(_dVars.targetBlockBottom, _dVars.targetBlockTop);
    if(_dVars.targetBlockTop.IsSet() && _dVars.targetBlockBottom.IsSet()){
      TransitionToPickingUpBlock();
    }
  };
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  // If cozmo thinks he's still carrying the cube, try placing it on the ground to
  // see if it's really there - then see if we can try again
  if(robotInfo.GetCarryingComponent().IsCarryingObject()){
    CompoundActionSequential* placeAction = new CompoundActionSequential({
      new DriveStraightAction(-kDistToBackupOnStackFailure_mm,
                              DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
      new PlaceObjectOnGroundAction()});
    DelegateIfInControl(placeAction, retryIfPossible);
  }else{
    retryIfPossible();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorStackBlocks::GetClosestValidBottom(ObjectInteractionIntention bottomIntention) const
{
  DEV_ASSERT(_dVars.targetBlockBottom.IsSet(),
             "BehaviorStackBlocks.GetClosestValidBottom.TargetBlockNotValid");
  
  auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();

  ObjectID bestBottom = _dVars.targetBlockBottom;
  const ObservableObject* currentTarget =  GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetBlockBottom);
  f32 distToClosestBottom_mm_sq;
  
  const auto& robotInfo = GetBEI().GetRobotInfo();

  if((currentTarget != nullptr) &&
     ComputeDistanceSQBetween(robotInfo.GetPose(), currentTarget->GetPose(), distToClosestBottom_mm_sq)){
    auto validObjs = objInfoCache.GetValidObjectsForIntention(bottomIntention);
    for(const auto& objID : validObjs){
      const ObservableObject* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(objID);
      f32 distanceToValid;
      if((obj != nullptr) &&
         ComputeDistanceSQBetween(robotInfo.GetPose(), obj->GetPose(), distanceToValid)){
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
void BehaviorStackBlocks::SetState_internal(State state, const std::string& stateName)
{
  _dVars.behaviorState = state;
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
                    GetBEI().GetRobotInfo().GetDockingComponent().CanPickUpObject(*obj),
                    GetBEI().GetRobotInfo().GetDockingComponent().CanStackOnTopOfObject(*obj),
                    poseStateStr,
                    obj->IsMoving(),
                    obj->IsRestingFlat());
}


} // namespace Cozmo
} // namespace Anki
