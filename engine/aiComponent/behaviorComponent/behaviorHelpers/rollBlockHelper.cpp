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
RollBlockHelper::RollBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                 ICozmoBehavior& behavior,
                                 BehaviorHelperFactory& helperFactory,
                                 const ObjectID& targetID,
                                 bool rollToUpright,
                                 const RollBlockParameters& parameters)
: IHelper("RollBlock", behaviorExternalInterface, behavior, helperFactory)
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
bool RollBlockHelper::ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus RollBlockHelper::InitBehaviorHelper(BehaviorExternalInterface& behaviorExternalInterface)
{
  // do first update immediately
  _shouldRoll = true;
  _tmpRetryCounter = 0;
  DetermineAppropriateAction(behaviorExternalInterface);
  return _status;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus RollBlockHelper::UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::DetermineAppropriateAction(BehaviorExternalInterface& behaviorExternalInterface)
{
  bool carryingObject = false;
  {
    CarryingComponent& carryingComp = behaviorExternalInterface.GetRobotInfo().GetCarryingComponent();
    carryingObject = carryingComp.IsCarryingObject();
  }
  // If the robot is carrying a block, put it down
  if(carryingObject){
    DelegateToPutDown(behaviorExternalInterface);
  }else{
    // If the block can't be accessed, pick it up and move it so it can be rolled
    const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
    if(obj != nullptr){
      DockingComponent& dockComp = behaviorExternalInterface.GetRobotInfo().GetDockingComponent();
      const bool canActionRoll = RollObjectAction::CanActionRollObject(dockComp, obj);

      if(canActionRoll){
        PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.Update.Rolling", "Doing roll action");
        const ActionResult isAtPreAction = IsAtPreActionPoseWithVisualVerification(
                                   behaviorExternalInterface, _targetID, PreActionPose::ActionType::ROLLING);
        
        if(isAtPreAction != ActionResult::SUCCESS){
          DriveToParameters params;
          params.actionType = PreActionPose::ActionType::ROLLING;
          
          // If rolling to upright, set the drive to helper approach angle if possible
          f32 uprightApproachAngle_rad;
          const auto& robotPose = behaviorExternalInterface.GetRobotInfo().GetPose();
          const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
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
          delegateProperties.SetDelegateToSet(CreateDriveToHelper(behaviorExternalInterface, _targetID, params));
          delegateProperties.SetOnSuccessFunction([this](BehaviorExternalInterface& behaviorExternalInterface)
                                  {StartRollingAction(behaviorExternalInterface); return _status;});
          delegateProperties.SetOnFailureFunction([this](BehaviorExternalInterface& behaviorExternalInterface)
                                  {DetermineAppropriateAction(behaviorExternalInterface); return _status;});
          DelegateAfterUpdate(delegateProperties);
        }else{
          StartRollingAction(behaviorExternalInterface);
        }
      }else{
        UnableToRollDelegate(behaviorExternalInterface);
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
void RollBlockHelper::UnableToRollDelegate(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr) {
    auto& objInfoCache = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache();

    const bool canPickup = objInfoCache.IsObjectValidForInteraction(
                             ObjectInteractionIntention::PickUpObjectNoAxisCheck,
                             obj->GetID());
    
    // See if any blocks are on top of the one we want to roll
    BlockWorldFilter blocksOnlyFilter;
    blocksOnlyFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    const ObservableObject* objOnTop = behaviorExternalInterface.GetBlockWorld().FindLocatedObjectOnTopOf(
                                              *obj,
                                              BlockWorld::kOnCubeStackHeightTolerance,
                                              blocksOnlyFilter);
    
    if(canPickup){
      PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.PickingUpTargetObject",
                    "Picking up the target object %d to roll it once moved",
                    obj->GetID().GetValue());
      // If we can pickup the cube, pick it up, put it down somewhere we can roll it
      // and then roll it
      auto pickupHelper = CreatePickupBlockHelper(behaviorExternalInterface, _targetID);
      DelegateProperties delegateProperties;
      delegateProperties.SetDelegateToSet(pickupHelper);
      delegateProperties.SetOnSuccessFunction([this](BehaviorExternalInterface& behaviorExternalInterface)
                                              {DelegateToPutDown(behaviorExternalInterface); return _status;});

      delegateProperties.SetOnFailureFunction( [](BehaviorExternalInterface& behaviorExternalInterface)
                                              {return IHelper::HelperStatus::Failure;});
      DelegateAfterUpdate(delegateProperties);
    }else if(objOnTop != nullptr){
      PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.PickingUpObjOnTop",
                    "Picking up object %d which is sitting on top of target block",
                    objOnTop->GetID().GetValue());
      // The cube can't be rolled b/c it's under another cube - pickup the top
      // cube and then we can roll the one it's beneath
      auto pickupHelper = CreatePickupBlockHelper(behaviorExternalInterface, objOnTop->GetID());
      DelegateProperties delegateProperties;
      delegateProperties.SetDelegateToSet(pickupHelper);
      delegateProperties.SetOnSuccessFunction([this](BehaviorExternalInterface& behaviorExternalInterface)
                                              {DelegateToPutDown(behaviorExternalInterface); return _status;});

      delegateProperties.SetOnFailureFunction( [](BehaviorExternalInterface& behaviorExternalInterface)
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
void RollBlockHelper::DelegateToPutDown(BehaviorExternalInterface& behaviorExternalInterface)
{
  DelegateProperties delegateProperties;
  delegateProperties.SetDelegateToSet(CreatePlaceBlockHelper(behaviorExternalInterface));
  delegateProperties.SetOnSuccessFunction(
                                          [this](BehaviorExternalInterface& behaviorExternalInterface) {
                                            _shouldRoll = true;
                                            DetermineAppropriateAction(behaviorExternalInterface);
                                            return IHelper::HelperStatus::Running;
                                          });
  delegateProperties.SetOnFailureFunction([](BehaviorExternalInterface& behaviorExternalInterface)
                                          { return IHelper::HelperStatus::Failure;});
  DelegateAfterUpdate(delegateProperties);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::StartRollingAction(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(_tmpRetryCounter >= kMaxNumRetrys){
    MarkTargetAsFailedToRoll(behaviorExternalInterface);
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
    const Pose3d& pose = behaviorExternalInterface.GetRobotInfo().GetPose();
    
    rollAction->RollToUpright(behaviorExternalInterface.GetBlockWorld(), pose);
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
void RollBlockHelper::RespondToRollingResult(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface)
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
      StartRollingAction(behaviorExternalInterface);
      break;
    }
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::MarkTargetAsFailedToRoll(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr){
    auto& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
    whiteboard.SetFailedToUse(*obj, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
  }
}

  
} // namespace Cozmo
} // namespace Anki

