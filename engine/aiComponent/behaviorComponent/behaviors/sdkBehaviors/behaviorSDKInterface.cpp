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


#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/sdkBehaviors/behaviorSDKInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorDriveOffCharger.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorGoHome.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sdkComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robotEventHandler.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* const kDriveOffChargerBehaviorKey = "driveOffChargerBehavior";
const char* const kGoHomeBehaviorKey = "goHomeBehavior";
}

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
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig.driveOffChargerBehaviorStr = JsonTools::ParseString(config, kDriveOffChargerBehaviorKey, debugName);
  _iConfig.goHomeBehaviorStr = JsonTools::ParseString(config, kGoHomeBehaviorKey, debugName);

  SubscribeToAppTags({
    AppToEngineTag::kDriveOffChargerRequest,
    AppToEngineTag::kDriveOnChargerRequest,
  });
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
  delegates.insert(_iConfig.driveOffChargerBehavior.get());
  delegates.insert(_iConfig.goHomeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.driveOffChargerBehaviorStr));
  DEV_ASSERT(_iConfig.driveOffChargerBehavior != nullptr,
             "BehaviorFindFaces.InitBehavior.NullDriveOffChargerBehavior");

  _iConfig.goHomeBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.goHomeBehaviorStr));
  DEV_ASSERT(_iConfig.goHomeBehavior != nullptr,
             "BehaviorFindFaces.InitBehavior.NullGoHomeBehavior");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kDriveOffChargerBehaviorKey,
    kGoHomeBehaviorKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
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
  if (!IsActivated()) {
    return;
  }

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

void BehaviorSDKInterface::HandleDriveOffChargerComplete() {
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOffChargerResult = new external_interface::DriveOffChargerResult;
    driveOffChargerResult->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOffChargerResult) );
  }
}  

void BehaviorSDKInterface::HandleDriveOnChargerComplete() {
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOnChargerResult = new external_interface::DriveOnChargerResult;
    driveOnChargerResult->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOnChargerResult) );
  }
}

void BehaviorSDKInterface::HandleWhileActivated(const AppToEngineEvent& event) {
  if( event.GetData().GetTag() == external_interface::GatewayWrapperTag::kDriveOffChargerRequest ) {
     DriveOffChargerRequest(event.GetData().drive_off_charger_request());
  } else if( event.GetData().GetTag() == external_interface::GatewayWrapperTag::kDriveOnChargerRequest ) {
     DriveOnChargerRequest(event.GetData().drive_on_charger_request());
  }  
}

// Delegate to the DriveOffCharger behavior
void BehaviorSDKInterface::DriveOffChargerRequest(const external_interface::DriveOffChargerRequest& driveOffChargerRequest) {
  if (_iConfig.driveOffChargerBehavior->WantsToBeActivated()) {
    if (DelegateIfInControl(_iConfig.driveOffChargerBehavior.get(), &BehaviorSDKInterface::HandleDriveOffChargerComplete)) {
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOffChargerResult = new external_interface::DriveOffChargerResult;
    driveOffChargerResult->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOffChargerResult) );
  }
}

// Delegate to GoHome
void BehaviorSDKInterface::DriveOnChargerRequest(const external_interface::DriveOnChargerRequest& driveOnChargerRequest) {
  if (_iConfig.goHomeBehavior->WantsToBeActivated()) {
    if (DelegateIfInControl(_iConfig.goHomeBehavior.get(), &BehaviorSDKInterface::HandleDriveOnChargerComplete)) {
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOnChargerResult = new external_interface::DriveOnChargerResult;
    driveOnChargerResult->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOnChargerResult) );
  }
}
} // namespace Cozmo
} // namespace Anki
