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
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::IHelper(const std::string& name,
                 Robot& robot,
                 IBehavior* behavior,
                 BehaviorHelperFactory& helperFactory)
: _status(IBehavior::Status::Complete)
, _hasStarted(false)
, _onSuccessFunction(nullptr)
, _onFailureFunction(nullptr)
, _name(name)
, _behaviorToCallActionsOn(behavior)
, _helperFactory(helperFactory)
{
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status IHelper::UpdateWhileActive(Robot& robot, HelperHandle& delegateToSet)
{
  DelegateProperties properties;

  bool tickUpdate = true;
  if(!_hasStarted){
    PRINT_CH_INFO("Behaviors", "IHelper.Init", "%s", GetName().c_str());

    _hasStarted = true;
    _status = Init(robot, properties);
    // If a delegate has been set, don't tick update while active
    if(_status != IBehavior::Status::Running ||
       properties.GetDelegateToSet() != nullptr){

      tickUpdate = false;
    }
  }

  if( tickUpdate ) {
    _status = UpdateWhileActiveInternal(robot, properties);
  }

  if( properties.GetDelegateToSet() ) {
    delegateToSet = properties.GetDelegateToSet();
    _onSuccessFunction = properties.GetOnSuccessFunction();
    _onFailureFunction = properties.GetOnFailureFunction();
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
  return _behaviorToCallActionsOn->IsActing();
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
    _behaviorToCallActionsOn->StopActing(allowCallback, keepHelpers);
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
  return _behaviorToCallActionsOn->StartActing(action, callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle IHelper::CreatePickupBlockHelper(Robot& robot, const ObjectID& targetID)
{
  return _helperFactory.CreatePickupBlockHelper(robot, _behaviorToCallActionsOn, targetID);
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

  
  
} // namespace Cozmo
} // namespace Anki

