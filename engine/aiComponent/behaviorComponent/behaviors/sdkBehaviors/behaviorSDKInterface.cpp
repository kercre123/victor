/**
 * File: BehaviorSDKInterface.cpp
 *
 * Author: Michelle Sintov
 * Created: 2018-05-21
 *
 * Description: Interface for SDKs including C# and Python
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/sdkBehaviors/behaviorSDKInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sdkComponent.h"
#include "engine/robotEventHandler.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSDKInterface::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSDKInterface::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSDKInterface::BehaviorSDKInterface(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSDKInterface::WantsToBeActivatedBehavior() const
{  
  // TODO Check whether the SDK wants the behavior slot that this behavior instance is for. Currently just using SDK0.
  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& sdkComponent = robotInfo.GetSDKComponent();
  return sdkComponent.SDKWantsControl();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  //const char* list[] = {
    // TODO: insert any possible root-level json keys that this class is expecting.
    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
  //};
  // expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  // Permit actions and low level motor control to run since SDK behavior is now active.
  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& robotEventHandler = robotInfo.GetRobotEventHandler();  
  robotEventHandler.SetAllowedToHandleActions(true);

  auto& movementComponent = robotInfo.GetMoveComponent();
  movementComponent.SetAllowedToHandleActions(true);

  // Tell the robot component that the SDK has been activated
  auto& sdkComponent = robotInfo.GetSDKComponent();
  sdkComponent.SDKBehaviorActivation(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::OnBehaviorDeactivated()
{
  // Tell the robot component that the SDK has been deactivated
  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& sdkComponent = robotInfo.GetSDKComponent();
  sdkComponent.SDKBehaviorActivation(false);

  // Do not permit actions and low level motor control to run since SDK behavior is no longer active.
  auto& robotEventHandler = robotInfo.GetRobotEventHandler();
  robotEventHandler.SetAllowedToHandleActions(false);

  auto& movementComponent = robotInfo.GetMoveComponent();
  movementComponent.SetAllowedToHandleActions(false);
}    

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::BehaviorUpdate() 
{
  if( IsActivated() ) {
    // TODO Consider which slot should be deactivated once SDK occupies multiple slots.
    //
    // TODO If the external SDK code (e.g., Python) crashes or otherwise does not release
    // control, then currently behavior will be activated indefinitely until a higher
    // priority behavior takes over.
    auto& robotInfo = GetBEI().GetRobotInfo();
    auto& sdkComponent = robotInfo.GetSDKComponent();
    if (!sdkComponent.SDKWantsControl())
    {
      CancelSelf();
    }
  }
}

}
}
