/**
 * File: iHelper.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Interface for BehaviorHelpers
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"

#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::DelegateProperties::ClearDelegateProperties()
{
  _delegateToSet.reset();
  _onSuccessFunction = nullptr;
  _onFailureFunction = nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::IHelper(const std::string& name,
                 Robot& robot,
                 IBehavior& behavior,
                 BehaviorHelperFactory& helperFactory)
:_status(IBehavior::Status::Complete)
, _name(name)
, _hasStarted(false)
, _onSuccessFunction(nullptr)
, _onFailureFunction(nullptr)
, _behaviorToCallActionsOn(behavior)
, _helperFactory(helperFactory)
{
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status IHelper::UpdateWhileActive(Robot& robot, HelperHandle& delegateToSet)
{
  
  bool tickUpdate = true;
  if(!_hasStarted){
    PRINT_CH_INFO("Behaviors", "IHelper.Init", "%s", GetName().c_str());

    _hasStarted = true;
    _status = Init(robot);
    // If a delegate has been set, don't tick update while active
    if(_status != IBehavior::Status::Running ||
       _delegateAfterUpdate.GetDelegateToSet() != nullptr){
      tickUpdate = false;
    }
  }

  if( tickUpdate ) {
    _status = UpdateWhileActiveInternal(robot);
  }
  
  if(_delegateAfterUpdate.GetDelegateToSet() != nullptr){
    delegateToSet = _delegateAfterUpdate.GetDelegateToSet();
    _onSuccessFunction = _delegateAfterUpdate.GetOnSuccessFunction();
    _onFailureFunction = _delegateAfterUpdate.GetOnFailureFunction();
    _delegateAfterUpdate.ClearDelegateProperties();
  }
  
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::InitializeOnStack()
{
  _status = IBehavior::Status::Running;
  _hasStarted = false;
  _onSuccessFunction = nullptr;
  _onFailureFunction = nullptr;
  InitializeOnStackInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::IsActing()
{
  return _behaviorToCallActionsOn.IsActing();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::Stop(bool isActive)
{
  PRINT_CH_INFO("Behaviors", "IHelper.Stop", "%s isActive=%d, IsActing=%d",
                GetName().c_str(),
                isActive,
                IsActing());
  
  // assumption: if the behavior is acting, and we are active, then we must have started the action, so we
  // should stop it
  if( isActive && IsActing() ) {
    const bool allowCallback = false;
    const bool keepHelpers = true; // to avoid infinite loops of Stop
    _behaviorToCallActionsOn.StopActing(allowCallback, keepHelpers);
  }

  StopInternal(isActive);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status IHelper::OnDelegateSuccess(Robot& robot)
{
  PRINT_CH_INFO("Behaviors", "IHelper.OnDelegateSuccess", "%s",
                GetName().c_str());

  if(_onSuccessFunction != nullptr){
    _status = _onSuccessFunction(robot);
  }

  // callbacks only happen once per delegate, so clear it after we call it
  _onSuccessFunction = nullptr;
  
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status IHelper::OnDelegateFailure(Robot& robot)
{
  PRINT_CH_INFO("Behaviors", "IHelper.OnDelegateFailure", "%s",
                GetName().c_str());

  if(_onFailureFunction != nullptr) {
    _status = _onFailureFunction(robot);
  }

  // callbacks only happen once per delegate, so clear it after we call it
  _onSuccessFunction = nullptr;
  
  return _status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::StartActing(IActionRunner* action, IBehavior::ActionResultWithRobotCallback callback)
{
  return _behaviorToCallActionsOn.StartActing(action, callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePickupBlockHelper(Robot& robot, const ObjectID& targetID, AnimationTrigger animBeforeDock)
{
  return _helperFactory.CreatePickupBlockHelper(robot, _behaviorToCallActionsOn, targetID, animBeforeDock);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePlaceBlockHelper(Robot& robot)
{
  return _helperFactory.CreatePlaceBlockHelper(robot, _behaviorToCallActionsOn);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateRollBlockHelper(Robot& robot, const ObjectID& targetID, bool rollToUpright)
{
  return _helperFactory.CreateRollBlockHelper(robot, _behaviorToCallActionsOn, targetID, rollToUpright);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateDriveToHelper(Robot& robot,
                                 const ObjectID& targetID,
                                 const PreActionPose::ActionType& actionType)
{
  return _helperFactory.CreateDriveToHelper(robot, _behaviorToCallActionsOn, targetID, actionType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePlaceRelObjectHelper(Robot& robot,
                                        const ObjectID& targetID,
                                        const bool placingOnGround,
                                        const f32 placementOffsetX_mm,
                                        const f32 placementOffsetY_mm,
                                        const bool relativeCurrentMarker)
{
  return _helperFactory.CreatePlaceRelObjectHelper(robot, _behaviorToCallActionsOn,
                                                   targetID, placingOnGround,
                                                   placementOffsetX_mm,
                                                   placementOffsetY_mm,
                                                   relativeCurrentMarker);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult IHelper::IsAtPreActionPose(Robot& robot, const ObjectID& targetID, PreActionPose::ActionType actionType)
{
  ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(targetID);
  // TMP - should check cast here
  ActionableObject* object = dynamic_cast<ActionableObject*>(staticBlock);
  const IDockAction::PreActionPoseInput preActionPoseInput(object,
                                                           actionType,
                                                           false,
                                                           0,
                                                           DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE,
                                                           false,
                                                           0);
  
  IDockAction::PreActionPoseOutput preActionPoseOutput;
  
  IDockAction::GetPreActionPoses(robot, preActionPoseInput, preActionPoseOutput);
  
  if(preActionPoseOutput.actionResult != ActionResult::SUCCESS)
  {
    return preActionPoseOutput.actionResult;
  }
  
  bool alreadyInPosition = preActionPoseOutput.robotAtClosestPreActionPose;
  if(alreadyInPosition){
    return ActionResult::SUCCESS;
  }else{
    return ActionResult::DID_NOT_REACH_PREACTION_POSE;
  }

}

  
} // namespace Cozmo
} // namespace Anki

