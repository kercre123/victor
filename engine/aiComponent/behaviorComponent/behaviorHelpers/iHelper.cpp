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


#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"


#include "engine/actions/animActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/robot.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const float kLastObservedTimestampTolerance_ms = 1000.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::DelegateProperties::ClearDelegateProperties()
{
  _delegateToSet.reset();
  _onSuccessFunction = nullptr;
  _onFailureFunction = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::DelegateProperties::SucceedImmediatelyOnDelegateFailure()
{
  SetOnSuccessFunction( [](BehaviorExternalInterface& behaviorExternalInterface) { return ICozmoBehavior::Status::Complete; } );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::DelegateProperties::FailImmediatelyOnDelegateFailure()
{
  SetOnFailureFunction( [](BehaviorExternalInterface& behaviorExternalInterface) { return ICozmoBehavior::Status::Failure; } );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::IHelper(const std::string& name,
                 BehaviorExternalInterface& behaviorExternalInterface,
                 ICozmoBehavior& behavior,
                 BehaviorHelperFactory& helperFactory)
: IBehavior(name)
, _status(ICozmoBehavior::Status::Complete)
, _name(name)
, _hasStarted(false)
, _onSuccessFunction(nullptr)
, _onFailureFunction(nullptr)
, _behaviorToCallActionsOn(behavior)
, _helperFactory(helperFactory)
{
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status IHelper::UpdateWhileActive(BehaviorExternalInterface& behaviorExternalInterface, HelperHandle& delegateToSet)
{
  
  bool tickUpdate = true;
  if(!_hasStarted){
    Util::sEventF("robot.behavior_helper.start", {}, "%s", GetName().c_str());
    PRINT_CH_INFO("BehaviorHelpers", "IHelper.Init", "%s", GetName().c_str());

    _hasStarted = true;
    _timeStarted_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _status = Init(behaviorExternalInterface);
    // If a delegate has been set, don't tick update while active
    if(_status != ICozmoBehavior::Status::Running ||
       _delegateAfterUpdate.GetDelegateToSet() != nullptr){
      tickUpdate = false;
    }
  }

  if( tickUpdate ) {
    _status = UpdateWhileActiveInternal(behaviorExternalInterface);
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
void IHelper::OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  _status = ICozmoBehavior::Status::Running;
  _hasStarted = false;
  _onSuccessFunction = nullptr;
  _onFailureFunction = nullptr;
  OnActivatedHelper(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::IsControlDelegated()
{
  return _behaviorToCallActionsOn.IsControlDelegated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::Stop(bool isActive)
{
  PRINT_CH_INFO("BehaviorHelpers", "IHelper.Stop", "%s isActive=%d, IsActing=%d",
                GetName().c_str(),
                isActive,
                IsControlDelegated());

  LogStopEvent(isActive);
  
  // assumption: if the behavior is acting, and we are active, then we must have started the action, so we
  // should stop it
  if( isActive && _behaviorToCallActionsOn.IsActing() ) {
    const bool allowCallback = false;
    const bool keepHelpers = true; // to avoid infinite loops of Stop
    _behaviorToCallActionsOn.StopActing(allowCallback, keepHelpers);
  }

  StopInternal(isActive);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::LogStopEvent(bool isActive)
{
  auto logEventWithName = [this](const std::string& event) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    int nSecs = Util::numeric_cast<int>(currTime_s - _timeStarted_s);
    if( nSecs < 0 ) {
      PRINT_NAMED_ERROR("IHelper.Stop.InvalidTime",
                        "%s: Negative duration %i secs (started %f, stopped %f)",
                        GetName().c_str(),
                        nSecs,
                        _timeStarted_s,
                        currTime_s);
      nSecs = 0;
    }

    Util::sEventF(event.c_str(),
                  {{DDATA, std::to_string(nSecs).c_str()}},
                  "%s", GetName().c_str());
  };    

  switch( _status ) {
    case ICozmoBehavior::Status::Complete: {
      logEventWithName("robot.behavior_helper.success");
      break;
    }

    case ICozmoBehavior::Status::Failure: {
      logEventWithName("robot.behavior_helper.failure");
      break;
    }

    case ICozmoBehavior::Status::Running:
      // if we were running, then we must have been canceled. If we were active, then we were canceled
      // directly, if we are not active, we were canceled as part of the stack being cleared
      if( isActive ) {
        logEventWithName("robot.behavior_helper.cancel");
      }
      else {
        logEventWithName("robot.behavior_helper.inactive_stop");
      }
      
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status IHelper::OnDelegateSuccess(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_CH_DEBUG("BehaviorHelpers", "IHelper.OnDelegateSuccess", "%s",
                 GetName().c_str());

  if(_onSuccessFunction != nullptr){
    _status = _onSuccessFunction(behaviorExternalInterface);
  }

  // callbacks only happen once per delegate, so clear it after we call it
  _onSuccessFunction = nullptr;
  
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status IHelper::OnDelegateFailure(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_CH_INFO("BehaviorHelpers", "IHelper.OnDelegateFailure", "%s",
                GetName().c_str());

  if(_onFailureFunction != nullptr) {
    _status = _onFailureFunction(behaviorExternalInterface);
  }

  // callbacks only happen once per delegate, so clear it after we call it
  _onSuccessFunction = nullptr;
  
  return _status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::DelegateIfInControl(IActionRunner* action, ICozmoBehavior::ActionResultWithRobotCallback callback)
{
  return _behaviorToCallActionsOn.DelegateIfInControl(action, callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::DelegateIfInControl(IActionRunner* action, BehaviorRobotCompletedActionWithExternalInterfaceCallback callback)
{
  return _behaviorToCallActionsOn.DelegateIfInControl(action, callback);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::StopActing(bool allowCallback)
{
  const bool allowHelperToContinue = true;
  return _behaviorToCallActionsOn.StopActing(allowCallback, allowHelperToContinue);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePickupBlockHelper(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& targetID, const PickupBlockParamaters& parameters)
{
  return _helperFactory.CreatePickupBlockHelper(behaviorExternalInterface, _behaviorToCallActionsOn, targetID, parameters);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePlaceBlockHelper(BehaviorExternalInterface& behaviorExternalInterface)
{
  return _helperFactory.CreatePlaceBlockHelper(behaviorExternalInterface, _behaviorToCallActionsOn);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateRollBlockHelper(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& targetID, bool rollToUpright, const RollBlockParameters& parameters)
{
  return _helperFactory.CreateRollBlockHelper(behaviorExternalInterface, _behaviorToCallActionsOn, targetID, rollToUpright, parameters);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateDriveToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                          const ObjectID& targetID,
                                          const DriveToParameters& parameters)
{
  return _helperFactory.CreateDriveToHelper(behaviorExternalInterface, _behaviorToCallActionsOn, targetID, parameters);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePlaceRelObjectHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                 const ObjectID& targetID,
                                                 const bool placingOnGround,
                                                 const PlaceRelObjectParameters& parameters)
{
  return _helperFactory.CreatePlaceRelObjectHelper(behaviorExternalInterface, _behaviorToCallActionsOn,
                                                   targetID, placingOnGround,
                                                   parameters);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateSearchForBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                 const SearchParameters& params)
{
  return _helperFactory.CreateSearchForBlockHelper(behaviorExternalInterface, _behaviorToCallActionsOn, params);
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult IHelper::IsAtPreActionPoseWithVisualVerification(BehaviorExternalInterface& behaviorExternalInterface,
                                                              const ObjectID& targetID,
                                                              PreActionPose::ActionType actionType,
                                                              const f32 offsetX_mm,
                                                              const f32 offsetY_mm)
{
  const ActionableObject* object = dynamic_cast<const ActionableObject*>(
                                behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(targetID));
  if(object == nullptr){
    return ActionResult::BAD_OBJECT;
  }
  
  // Check that the object has been visually verified within the last second
  const TimeStamp_t currentTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  if(object->GetLastObservedTime() + kLastObservedTimestampTolerance_ms < currentTimestamp){
    PRINT_CH_INFO("BehaviorHelpers",
                  "IHelper.IsAtPreActionPoseWithVisualVerification.VisualVerifyFailed",
                  "Helper %s failed to visually verify object %d",
                  GetName().c_str(), targetID.GetValue());
    return ActionResult::VISUAL_OBSERVATION_FAILED;
  }
  
  bool alreadyInPosition = false;
  
  if(actionType == PreActionPose::ActionType::PLACE_RELATIVE)
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    std::vector<Pose3d> possiblePoses_unused;
    PlaceRelObjectAction::ComputePlaceRelObjectOffsetPoses(object,
                                                           offsetX_mm,
                                                           offsetY_mm,
                                                           robot,
                                                           possiblePoses_unused,
                                                           alreadyInPosition);
  }
  else
  {
    const IDockAction::PreActionPoseInput preActionPoseInput(object,
                                                             actionType,
                                                             false,
                                                             0,
                                                             DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE,
                                                             false,
                                                             0);
    
    IDockAction::PreActionPoseOutput preActionPoseOutput;
    
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    IDockAction::GetPreActionPoses(robot, preActionPoseInput, preActionPoseOutput);
    
    if(preActionPoseOutput.actionResult != ActionResult::SUCCESS)
    {
      return preActionPoseOutput.actionResult;
    }
    
    alreadyInPosition = preActionPoseOutput.robotAtClosestPreActionPose;
  }
  
  if(alreadyInPosition){
    return ActionResult::SUCCESS;
  }else{
    return ActionResult::DID_NOT_REACH_PREACTION_POSE;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename T>
void IHelper::RespondToActionWithAnim(const T& res, ActionResult actionResult,
                                      BehaviorExternalInterface& behaviorExternalInterface, std::function<void(const T&, BehaviorExternalInterface&)>& callback)
{
  PRINT_CH_INFO("BehaviorHelpers",
                "IHelper.RespondToResultWithAnim.ActionResult",
                "Action completed with result %s - playing appropriate response",
                EnumToString(actionResult));
  
  if(_actionResultMapFunc != nullptr)
  {
    UserFacingActionResult userResult = _actionResultMapFunc(actionResult);
    if(userResult != UserFacingActionResult::Count)
    {
      AnimationTrigger responseAnim = AnimationResponseToActionResult(behaviorExternalInterface, userResult);
      if(responseAnim != AnimationTrigger::Count)
      {
        // DEPRECATED - Grabbing robot to support current cozmo code, but this should
        // be removed
        Robot& robot = behaviorExternalInterface.GetRobot();
        DelegateIfInControl(new TriggerAnimationAction(robot, responseAnim),
                    [res, &callback](ActionResult animPlayed, BehaviorExternalInterface& behaviorExternalInterface){
                      // Pass through the true action result, not the played animation result
                      auto tmpCallback = callback;
                      callback = nullptr;
                      if(tmpCallback != nullptr){
                        tmpCallback(res, behaviorExternalInterface);
                      }
                    });
        
        _actionResultMapFunc = nullptr;
        return;
      }
    }
  }
  
  auto tmpCallback = callback;
  callback = nullptr;
  _actionResultMapFunc = nullptr;
  if(tmpCallback != nullptr){
    tmpCallback(res, behaviorExternalInterface);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::RespondToResultWithAnim(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface)
{
  RespondToActionWithAnim(result, result, behaviorExternalInterface, _callbackAfterResponseAnim);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::RespondToRCAWithAnim(const ExternalInterface::RobotCompletedAction& rca, BehaviorExternalInterface& behaviorExternalInterface)
{
  RespondToActionWithAnim(rca, rca.result, behaviorExternalInterface, _callbackAfterResponseAnimUsingRCA);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger IHelper::AnimationResponseToActionResult(BehaviorExternalInterface& behaviorExternalInterface, UserFacingActionResult result)
{
  return behaviorExternalInterface.GetAIComponent().GetBehaviorComponent().
              GetBehaviorEventAnimResponseDirector().GetAnimationToPlayForActionResult(result);
}

  
} // namespace Cozmo
} // namespace Anki

