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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sdkComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"

namespace Anki {
namespace Vector {

namespace {
const char* const kDriveOffChargerBehaviorKey = "driveOffChargerBehavior";
const char* const kFindAndGoToHomeBehaviorKey = "findAndGoToHomeBehavior";
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
  _iConfig.findAndGoToHomeBehaviorStr = JsonTools::ParseString(config, kFindAndGoToHomeBehaviorKey, debugName);

  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction,
  });

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
  delegates.insert(_iConfig.findAndGoToHomeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.driveOffChargerBehaviorStr));
  DEV_ASSERT(_iConfig.driveOffChargerBehavior != nullptr,
             "BehaviorFindFaces.InitBehavior.NullDriveOffChargerBehavior");

  _iConfig.findAndGoToHomeBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.findAndGoToHomeBehaviorStr));
  DEV_ASSERT(_iConfig.findAndGoToHomeBehavior != nullptr,
             "BehaviorFindFaces.InitBehavior.NullFindAndGoToHomeBehavior");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kDriveOffChargerBehaviorKey,
    kFindAndGoToHomeBehaviorKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSDKInterface::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  auto& robotInfo = GetBEI().GetRobotInfo();
  
  // Permit low level movement commands/actions to run since SDK behavior is now active.
  SetAllowExternalMovementCommands(true);

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

  // Do not permit low level movement commands/actions to run since SDK behavior is no longer active.
  SetAllowExternalMovementCommands(false);
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
  SetAllowExternalMovementCommands(true);
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOffChargerResponse = new external_interface::DriveOffChargerResponse;
    driveOffChargerResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOffChargerResponse) );
  }
}  

void BehaviorSDKInterface::HandleDriveOnChargerComplete() {
  SetAllowExternalMovementCommands(true);
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOnChargerResponse = new external_interface::DriveOnChargerResponse;
    driveOnChargerResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOnChargerResponse) );
  }
}

// Reports back to gateway that requested actions have been completed.
// E.g., the Python SDK ran play_anim and wants to know when the animation
// action was completed.
void BehaviorSDKInterface::HandleWhileActivated(const EngineToGameEvent& event)
{
  if (IsControlDelegated()) {
    // The SDK behavior has delegated to another behavior, and that
    // behavior requested an action. Don't inform gateway that the
    // action has completed because it wasn't requested by the SDK.
    //
    // If necessary, can delegate to actions from the behavior instead
    // of running them via CLAD request from gateway.
    return;
  }

  if (event.GetData().GetTag() != EngineToGameTag::RobotCompletedAction) {
    return;
  }

  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if (gi == nullptr) return;

  ExternalInterface::RobotCompletedAction msg = event.GetData().Get_RobotCompletedAction();
  switch((RobotActionType)msg.actionType)
  {
    case RobotActionType::TURN_IN_PLACE:
    {
      auto* response = new external_interface::TurnInPlaceResponse;
      response->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    case RobotActionType::DRIVE_STRAIGHT:
    {
      auto* response = new external_interface::DriveStraightResponse;
      response->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    case RobotActionType::MOVE_HEAD_TO_ANGLE:
    {
      auto* response = new external_interface::SetHeadAngleResponse;
      response->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    case RobotActionType::MOVE_LIFT_TO_HEIGHT:
    {
      auto* response = new external_interface::SetLiftHeightResponse;
      response->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    case RobotActionType::PLAY_ANIMATION:
    {
      auto* response = new external_interface::PlayAnimationResponse;
      response->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    default:
    {
      PRINT_NAMED_WARNING("BehaviorSDKInterface.HandleWhileActivated.NoMatch", "No match for action tag so no response sent: [Tag=%d]", msg.idTag);
      return;
    }
  }
}

void BehaviorSDKInterface::SetAllowExternalMovementCommands(const bool allow) {
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetMoveComponent().AllowExternalMovementCommands(allow, GetDebugLabel());
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
      SetAllowExternalMovementCommands(false);
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOffChargerResponse = new external_interface::DriveOffChargerResponse;
    driveOffChargerResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOffChargerResponse) );
  }
}

// Delegate to FindAndGoToHome
void BehaviorSDKInterface::DriveOnChargerRequest(const external_interface::DriveOnChargerRequest& driveOnChargerRequest) {
  if (_iConfig.findAndGoToHomeBehavior->WantsToBeActivated()) {
    if (DelegateIfInControl(_iConfig.findAndGoToHomeBehavior.get(), &BehaviorSDKInterface::HandleDriveOnChargerComplete)) {
      SetAllowExternalMovementCommands(false);
      return;
    }
  }

  // If we got this far, we failed to activate the requested behavior.
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* driveOnChargerResponse = new external_interface::DriveOnChargerResponse;
    driveOnChargerResponse->set_result(external_interface::BehaviorResults::BEHAVIOR_WONT_ACTIVATE_STATE);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(driveOnChargerResponse) );
  }
}
} // namespace Vector
} // namespace Anki
