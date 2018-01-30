/**
 * File: rollBlockHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handles rolling a block
 *
 * Copyright: Anki, Inc. 2017
 *
 **/



#include "engine/aiComponent/behaviorComponent/behaviorHelpers/rollBlockHelper.h"

#include "engine/actions/dockActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/pickupBlockHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/placeBlockHelper.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const int kMaxNumRetrys = 3;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RollBlockHelper::RollBlockHelper(ICozmoBehavior& behavior,
                                 BehaviorHelperFactory& helperFactory,
                                 const ObjectID& targetID,
                                 bool rollToUpright,
                                 const RollBlockParameters& parameters)
: IHelper("RollBlock", behavior, helperFactory)
, _targetID(targetID)
, _params(parameters)
, _shouldUpright(rollToUpright)
, _tmpRetryCounter(0)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RollBlockHelper::~RollBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RollBlockHelper::ShouldCancelDelegates() const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus RollBlockHelper::InitBehaviorHelper()
{
  // do first update immediately
  _shouldRoll = true;
  _tmpRetryCounter = 0;
  DetermineAppropriateAction();
  return _status;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus RollBlockHelper::UpdateWhileActiveInternal()
{
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::DetermineAppropriateAction()
{
  bool carryingObject = false;
  {
    CarryingComponent& carryingComp = GetBEI().GetRobotInfo().GetCarryingComponent();
    carryingObject = carryingComp.IsCarryingObject();
  }
  // If the robot is carrying a block, put it down
  if(carryingObject){
    DelegateToPutDown();
  }else{
    // If the block can't be accessed, pick it up and move it so it can be rolled
    const ObservableObject* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_targetID);
    if(obj != nullptr){
      DockingComponent& dockComp = GetBEI().GetRobotInfo().GetDockingComponent();
      const bool canActionRoll = RollObjectAction::CanActionRollObject(dockComp, obj);

      if(canActionRoll){
        PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.Update.Rolling", "Doing roll action");
        const ActionResult isAtPreAction = IsAtPreActionPoseWithVisualVerification(_targetID, PreActionPose::ActionType::ROLLING);
        
        if(isAtPreAction != ActionResult::SUCCESS){
          DriveToParameters params;
          params.actionType = PreActionPose::ActionType::ROLLING;
          
          // If rolling to upright, set the drive to helper approach angle if possible
          f32 uprightApproachAngle_rad;
          const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
          const auto& blockWorld = GetBEI().GetBlockWorld();
          if(_shouldUpright &&
             DriveToRollObjectAction::GetRollToUprightApproachAngle(blockWorld,
                                                                    robotPose,
                                                                    _targetID,
                                                                    uprightApproachAngle_rad)){
               params.useApproachAngle = true;
               params.approachAngle_rad = uprightApproachAngle_rad;
          }
          
          // Delegate to the drive to helper
          DelegateProperties delegateProperties;
          delegateProperties.SetDelegateToSet(CreateDriveToHelper(_targetID, params));
          delegateProperties.SetOnSuccessFunction([this]()
                                  {StartRollingAction(); return _status;});
          delegateProperties.SetOnFailureFunction([this]()
                                  {DetermineAppropriateAction(); return _status;});
          DelegateAfterUpdate(delegateProperties);
        }else{
          StartRollingAction();
        }
      }else{
        UnableToRollDelegate();
      }
      
    }else{
      PRINT_CH_INFO("BehaviorHelpers",
                    "RollBlockHelper.Update.NoObj",
                    "Failing helper, object %d is invalid",
                    _targetID.GetValue());
      _status = IHelper::HelperStatus::Failure;
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::UnableToRollDelegate()
{
  const ObservableObject* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr) {
    auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();

    const bool canPickup = objInfoCache.IsObjectValidForInteraction(
                             ObjectInteractionIntention::PickUpObjectNoAxisCheck,
                             obj->GetID());
    
    // See if any blocks are on top of the one we want to roll
    BlockWorldFilter blocksOnlyFilter;
    blocksOnlyFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    const ObservableObject* objOnTop = GetBEI().GetBlockWorld().FindLocatedObjectOnTopOf(
                                              *obj,
                                              BlockWorld::kOnCubeStackHeightTolerance,
                                              blocksOnlyFilter);
    
    if(canPickup){
      PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.PickingUpTargetObject",
                    "Picking up the target object %d to roll it once moved",
                    obj->GetID().GetValue());
      // If we can pickup the cube, pick it up, put it down somewhere we can roll it
      // and then roll it
      auto pickupHelper = CreatePickupBlockHelper( _targetID);
      DelegateProperties delegateProperties;
      delegateProperties.SetDelegateToSet(pickupHelper);
      delegateProperties.SetOnSuccessFunction([this]()
                                              {DelegateToPutDown(); return _status;});

      delegateProperties.SetOnFailureFunction( []()
                                              {return IHelper::HelperStatus::Failure;});
      DelegateAfterUpdate(delegateProperties);
    }else if(objOnTop != nullptr){
      PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.PickingUpObjOnTop",
                    "Picking up object %d which is sitting on top of target block",
                    objOnTop->GetID().GetValue());
      // The cube can't be rolled b/c it's under another cube - pickup the top
      // cube and then we can roll the one it's beneath
      auto pickupHelper = CreatePickupBlockHelper(objOnTop->GetID());
      DelegateProperties delegateProperties;
      delegateProperties.SetDelegateToSet(pickupHelper);
      delegateProperties.SetOnSuccessFunction([this]()
                                              {DelegateToPutDown(); return _status;});

      delegateProperties.SetOnFailureFunction( []()
                                              {return IHelper::HelperStatus::Failure;});
      DelegateAfterUpdate(delegateProperties);
    }else{
      PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.CantInteract",
                    "No way to interact with objID: %d defined",
                    _targetID.GetValue());
    }
  }else{
    PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.NoObj",
                  "Failing helper, object %d is invalid",
                  _targetID.GetValue());
    _status = IHelper::HelperStatus::Failure;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::DelegateToPutDown()
{
  DelegateProperties delegateProperties;
  delegateProperties.SetDelegateToSet(CreatePlaceBlockHelper());
  delegateProperties.SetOnSuccessFunction(
                                          [this]() {
                                            _shouldRoll = true;
                                            DetermineAppropriateAction();
                                            return IHelper::HelperStatus::Running;
                                          });
  delegateProperties.SetOnFailureFunction([]()
                                          { return IHelper::HelperStatus::Failure;});
  DelegateAfterUpdate(delegateProperties);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::StartRollingAction()
{
  if(_tmpRetryCounter >= kMaxNumRetrys){
    MarkTargetAsFailedToRoll();
    _status = IHelper::HelperStatus::Failure;
    return;
  }
  _tmpRetryCounter++;
  
  _shouldRoll = false;
  DriveToRollObjectAction* rollAction = nullptr;
  {
    rollAction = new DriveToRollObjectAction(_targetID);
  }
  
  if( _shouldUpright ) {
    const Pose3d& pose = GetBEI().GetRobotInfo().GetPose();
    
    rollAction->RollToUpright(GetBEI().GetBlockWorld(), pose);
  }
  if(_params.preDockCallback != nullptr){
    rollAction->SetPreDockCallback(_params.preDockCallback);
  }
  
  rollAction->SetSayNameAnimationTrigger(_params.sayNameAnimationTrigger);
  rollAction->SetNoNameAnimationTrigger(_params.noNameAnimationTrigger);
  rollAction->SetMaxTurnTowardsFaceAngle(_params.maxTurnToFaceAngle);
  

  DelegateWithResponseAnim(rollAction, &RollBlockHelper::RespondToRollingResult, [] (ActionResult result){
    switch(result){
      case ActionResult::SUCCESS:
      {
        return UserFacingActionResult::Success;
        break;
      }
      case ActionResult::LAST_PICK_AND_PLACE_FAILED:
      {
        return UserFacingActionResult::InteractWithBlockDockingIssue;
        break;
      }
      default:
      {
        return UserFacingActionResult::DriveToBlockIssue;
        break;
      }
    }
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::RespondToRollingResult(ActionResult result)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = IHelper::HelperStatus::Complete;
      break;
    }
    case ActionResult::CANCELLED_WHILE_RUNNING:
      // leave the helper running, since it's about to be canceled
      break;
    default:
    {
      StartRollingAction();
      break;
    }
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::MarkTargetAsFailedToRoll()
{
  const ObservableObject* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr){
    auto& whiteboard = GetBEI().GetAIComponent().GetWhiteboard();
    whiteboard.SetFailedToUse(*obj, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
  }
}

  
} // namespace Cozmo
} // namespace Anki

