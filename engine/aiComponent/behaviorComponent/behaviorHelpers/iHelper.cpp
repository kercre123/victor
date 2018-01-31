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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockWorld.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/logging/logging.h"

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
  SetOnSuccessFunction( []() { return IHelper::HelperStatus::Complete; } );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::DelegateProperties::FailImmediatelyOnDelegateFailure()
{
  SetOnFailureFunction( []() { return IHelper::HelperStatus::Failure; } );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::IHelper(const std::string& name,
                 ICozmoBehavior& behavior,
                 BehaviorHelperFactory& helperFactory)
: IBehavior(name)
, _status(IHelper::HelperStatus::Complete)
, _name(name)
, _hasStarted(false)
, _onSuccessFunction(nullptr)
, _onFailureFunction(nullptr)
, _behaviorToCallActionsOn(behavior)
, _helperFactory(helperFactory)
{
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus IHelper::UpdateWhileActive(HelperHandle& delegateToSet)
{
  
  bool tickUpdate = true;
  if(!_hasStarted){
    Util::sEventF("robot.behavior_helper.start", {}, "%s", GetName().c_str());
    PRINT_CH_INFO("BehaviorHelpers", "IHelper.Init", "%s", GetName().c_str());

    _hasStarted = true;
    _timeStarted_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _status = InitBehaviorHelper();
    // If a delegate has been set, don't tick update while active
    if(_status != IHelper::HelperStatus::Running ||
       _delegateAfterUpdate.GetDelegateToSet() != nullptr){
      tickUpdate = false;
    }
  }

  if( tickUpdate ) {
    _status = UpdateWhileActiveInternal();
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
void IHelper::InitInternal()
{
  // purposefully blank - only called to maintain iBehavior state info
}


  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::OnActivatedInternal()
{
  _status = IHelper::HelperStatus::Running;
  _hasStarted = false;
  _onSuccessFunction = nullptr;
  _onFailureFunction = nullptr;
  OnActivatedHelper();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::IsControlDelegated()
{
  return _behaviorToCallActionsOn.IsControlDelegated();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::IsActing() const
{
  return _behaviorToCallActionsOn.IsActing();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::Stop(bool isActive)
{
  PRINT_CH_INFO("BehaviorHelpers", "IHelper.Stop", "%s isActive=%d, IsControlDelegated=%d",
                GetName().c_str(),
                isActive,
                IsControlDelegated());

  LogStopEvent(isActive);
  
  // assumption: if the behavior is acting, and we are active, then we must have started the action, so we
  // should stop it
  if( isActive && _behaviorToCallActionsOn.IsActing() ) {    
    if(ANKI_VERIFY(_behaviorToCallActionsOn.GetBEI().HasDelegationComponent(),
                   "IHelper.Stop.NoDelegationComponent",
                   "")){
      auto& delegationComp = _behaviorToCallActionsOn.GetBEI().GetDelegationComponent();
      delegationComp.CancelActionIfRunning();
    }
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
    case IHelper::HelperStatus::Complete: {
      logEventWithName("robot.behavior_helper.success");
      break;
    }

    case IHelper::HelperStatus::Failure: {
      logEventWithName("robot.behavior_helper.failure");
      break;
    }

    case IHelper::HelperStatus::Running:
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
IHelper::HelperStatus IHelper::OnDelegateSuccess()
{
  PRINT_CH_DEBUG("BehaviorHelpers", "IHelper.OnDelegateSuccess", "%s",
                 GetName().c_str());

  if(_onSuccessFunction != nullptr){
    _status = _onSuccessFunction();
  }

  // callbacks only happen once per delegate, so clear it after we call it
  _onSuccessFunction = nullptr;
  
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus IHelper::OnDelegateFailure()
{
  PRINT_CH_INFO("BehaviorHelpers", "IHelper.OnDelegateFailure", "%s",
                GetName().c_str());

  if(_onFailureFunction != nullptr) {
    _status = _onFailureFunction();
  }

  // callbacks only happen once per delegate, so clear it after we call it
  _onSuccessFunction = nullptr;
  
  return _status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::DelegateIfInControl(IActionRunner* action, BehaviorActionResultCallback callback)
{
  return _behaviorToCallActionsOn.DelegateIfInControl(action, callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::DelegateIfInControl(IActionRunner* action, BehaviorRobotCompletedActionCallback callback)
{
  return _behaviorToCallActionsOn.DelegateIfInControl(action, callback);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IHelper::CancelDelegates(bool allowCallback)
{
  const bool allowHelperToContinue = true;
  return _behaviorToCallActionsOn.CancelDelegates(allowCallback, allowHelperToContinue);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePickupBlockHelper(const ObjectID& targetID, const PickupBlockParamaters& parameters)
{
  return _helperFactory.CreatePickupBlockHelper(_behaviorToCallActionsOn, targetID, parameters);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePlaceBlockHelper()
{
  return _helperFactory.CreatePlaceBlockHelper(_behaviorToCallActionsOn);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateRollBlockHelper(const ObjectID& targetID, bool rollToUpright, const RollBlockParameters& parameters)
{
  return _helperFactory.CreateRollBlockHelper(_behaviorToCallActionsOn, targetID, rollToUpright, parameters);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateDriveToHelper(const ObjectID& targetID,
                                          const DriveToParameters& parameters)
{
  return _helperFactory.CreateDriveToHelper(_behaviorToCallActionsOn, targetID, parameters);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePlaceRelObjectHelper(const ObjectID& targetID,
                                                 const bool placingOnGround,
                                                 const PlaceRelObjectParameters& parameters)
{
  return _helperFactory.CreatePlaceRelObjectHelper(_behaviorToCallActionsOn,
                                                   targetID, placingOnGround,
                                                   parameters);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateSearchForBlockHelper(const SearchParameters& params)
{
  return _helperFactory.CreateSearchForBlockHelper(_behaviorToCallActionsOn, params);
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult IHelper::IsAtPreActionPoseWithVisualVerification(const ObjectID& targetID,
                                                              PreActionPose::ActionType actionType,
                                                              const f32 offsetX_mm,
                                                              const f32 offsetY_mm)
{
  const ActionableObject* object = dynamic_cast<const ActionableObject*>(
                                GetBEI().GetBlockWorld().GetLocatedObjectByID(targetID));
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
    auto& robotInfo = GetBEI().GetRobotInfo();
    std::vector<Pose3d> possiblePoses_unused;
    PlaceRelObjectAction::ComputePlaceRelObjectOffsetPoses(object,
                                                           offsetX_mm,
                                                           offsetY_mm,
                                                           robotInfo.GetPose(),
                                                           robotInfo.GetWorldOrigin(),
                                                           robotInfo.GetCarryingComponent(),
                                                           GetBEI().GetBlockWorld(),
                                                           GetBEI().GetVisionComponent(),
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
    
    auto& robotInfo = GetBEI().GetRobotInfo();
    IDockAction::GetPreActionPoses(robotInfo.GetPose(),
                                   robotInfo.GetCarryingComponent(),
                                   GetBEI().GetBlockWorld(),
                                   preActionPoseInput, preActionPoseOutput);
    
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
                                      std::function<void(const T&)>& callback)
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
      AnimationTrigger responseAnim = AnimationResponseToActionResult(userResult);
      if(responseAnim != AnimationTrigger::Count)
      {
        DelegateIfInControl(new TriggerAnimationAction(responseAnim),
                    [res, &callback](ActionResult animPlayed){
                      // Pass through the true action result, not the played animation result
                      auto tmpCallback = callback;
                      callback = nullptr;
                      if(tmpCallback != nullptr){
                        tmpCallback(res);
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
    tmpCallback(res);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::RespondToResultWithAnim(ActionResult result)
{
  RespondToActionWithAnim(result, result, _callbackAfterResponseAnim);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IHelper::RespondToRCAWithAnim(const ExternalInterface::RobotCompletedAction& rca)
{
  RespondToActionWithAnim(rca, rca.result, _callbackAfterResponseAnimUsingRCA);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger IHelper::AnimationResponseToActionResult(UserFacingActionResult result)
{
  return GetBEI().GetAIComponent().GetBehaviorComponent().
              GetBehaviorEventAnimResponseDirector().GetAnimationToPlayForActionResult(result);
}

  
} // namespace Cozmo
} // namespace Anki

