/**
 * File: BehaviorSDKLock.cpp
 *
 * Author: Bruce von Kugelgen
 * Created: 2019-02-05
 *
 * Description: SDK Behavior for locking out other behaviors.  This can be used
 * to keep the robot still between running Python scripts.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/sdkBehaviors/behaviorSDKLock.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sdkComponent.h"

#define LOG_CHANNEL "SdkLock"

namespace Anki {
namespace Vector {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSDKLock::BehaviorSDKLock(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSDKLock::WantsToBeActivatedBehavior() const
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& sdkComponent = robotInfo.GetSDKComponent();
  return sdkComponent.SDKWantsBehaviorLock();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKLock::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKLock::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKLock::OnBehaviorActivated() 
{
  LOG_INFO("BehaviorSDKLock::OnBehaviorActivated","activated");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKLock::BehaviorUpdate() 
{
  if (!IsActivated()) {
    return;
  }

  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& sdkComponent = robotInfo.GetSDKComponent();
  if (!sdkComponent.SDKWantsBehaviorLock())
  {
    CancelSelf();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKLock::OnBehaviorDeactivated() 
{
  LOG_INFO("BehaviorSDKLock::OnBehaviorDeactivated","deactivated");
}

} //namespace Vector
} //namespace Anki
