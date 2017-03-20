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

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorEventAnimResponseDirector.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
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
    Util::sEventF("robot.behavior_helper.start", {}, "%s", GetName().c_str());
    PRINT_CH_INFO("Behaviors", "IHelper.Init", "%s", GetName().c_str());

    _hasStarted = true;
    _timeStarted_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
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

  LogStopEvent(isActive);
  
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
    case IBehavior::Status::Complete: {
      logEventWithName("robot.behavior_helper.success");
      break;
    }

    case IBehavior::Status::Failure: {
      logEventWithName("robot.behavior_helper.failure");
      break;
    }

    case IBehavior::Status::Running:
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
HelperHandle IHelper::CreatePickupBlockHelper(Robot& robot, const ObjectID& targetID, const PickupBlockParamaters& parameters)
{
  return _helperFactory.CreatePickupBlockHelper(robot, _behaviorToCallActionsOn, targetID, parameters);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePlaceBlockHelper(Robot& robot)
{
  return _helperFactory.CreatePlaceBlockHelper(robot, _behaviorToCallActionsOn);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateRollBlockHelper(Robot& robot, const ObjectID& targetID, bool rollToUpright, const RollBlockParameters& parameters)
{
  return _helperFactory.CreateRollBlockHelper(robot, _behaviorToCallActionsOn, targetID, rollToUpright, parameters);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreateDriveToHelper(Robot& robot,
                                 const ObjectID& targetID,
                                 const DriveToParameters& parameters)
{
  return _helperFactory.CreateDriveToHelper(robot, _behaviorToCallActionsOn, targetID, parameters);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePlaceRelObjectHelper(Robot& robot,
                                        const ObjectID& targetID,
                                        const bool placingOnGround,
                                        const PlaceRelObjectParameters& parameters)
{
  return _helperFactory.CreatePlaceRelObjectHelper(robot, _behaviorToCallActionsOn,
                                                   targetID, placingOnGround,
                                                   parameters);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult IHelper::IsAtPreActionPoseWithVisualVerification(Robot& robot,
                                                              const ObjectID& targetID,
                                                              PreActionPose::ActionType actionType,
                                                              const f32 offsetX_mm,
                                                              const f32 offsetY_mm)
{
  ActionableObject* object = dynamic_cast<ActionableObject*>(
                                robot.GetBlockWorld().GetLocatedObjectByID(targetID));
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
void IHelper::RespondToResultWithAnim(ActionResult result, Robot& robot)
{
  if(_actionResultMapFunc != nullptr){
    UserFacingActionResult userResult = _actionResultMapFunc(result);
    if(userResult != UserFacingActionResult::Count){
      AnimationTrigger responseAnim = AnimationResponseToActionResult(robot, userResult);
      if(responseAnim != AnimationTrigger::Count){
        StartActing(new TriggerAnimationAction(robot, responseAnim),
                    [this, &result](ActionResult animPlayed, Robot& robot){
          // Pass through the true action result, not the played animation result
          if(_callbackAfterResponseAnim != nullptr){
            _callbackAfterResponseAnim(result, robot);
          }
          _callbackAfterResponseAnim = nullptr;
        });
        _actionResultMapFunc = nullptr;
        return;
      }
    }
  }
  
  BehaviorActionResultWithRobotCallback tmpCallback = _callbackAfterResponseAnim;
  _callbackAfterResponseAnim = nullptr;
  _actionResultMapFunc = nullptr;
  if(tmpCallback != nullptr){
    tmpCallback(result, robot);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger IHelper::AnimationResponseToActionResult(Robot& robot, UserFacingActionResult result)
{
  return robot.GetAIComponent().GetBehaviorEventAnimResponseDirector().
                           GetAnimationToPlayForActionResult(result);
}

  
} // namespace Cozmo
} // namespace Anki

