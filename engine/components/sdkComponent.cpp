/**
 * File: sdkComponent
 *
 * Author: Michelle Sintov
 * Created: 05/25/2018
 *
 * Description: Component that serves as a mediator between external SDK requests and any instances of SDK behaviors,
 * such as SDK0.
 * 
 * The sdkComponent does the following, in this order:
 *     - The sdkComponent will receive a message from the external SDK, requesting that the SDK behavior be activated,
 *       so that the SDK can make the robot do something (which could be a behavior, action or low level motor control)
 *     - The SDK behavior will check the sdkComponent regularly to see if the SDK wants control.
 *       If so, the SDK behavior will say it wants to be activated.
 *     - When the SDK behavior is activated, it will inform sdkComponent that the SDK has control.
 *     - The sdkComponent sends a message to the external SDK to inform that it has control.
 *     - Any other state changes from the SDK behavior (e.g., lost control, etc.) are also communicated
 *       through the sdkComponent.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/ankiEventUtil.h"
#include "engine/components/sdkComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotEventHandler.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"

#include "util/logging/logging.h"

#define LOG_CHANNEL "SdkComponent"

namespace Anki {
namespace Vector {

SDKComponent::SDKComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::SDK)
{
}


SDKComponent::~SDKComponent()
{
}


void SDKComponent::GetInitDependencies( RobotCompIDSet& dependencies ) const
{
  // TODO
}


void SDKComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  // TODO It's preferred, where possible, to use dependentComps rather than caching robot
  // directly (makes it much easier to write unit tests)
  _robot = robot;
  auto* gi = robot->GetGatewayInterface();
  if (gi != nullptr)
  {
    auto callback = std::bind(&SDKComponent::HandleMessage, this, std::placeholders::_1);

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kControlRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kControlRelease, callback));

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kGoToPoseRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kDockWithCubeRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kDriveStraightRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kTurnInPlaceRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kSetLiftHeightRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kSetHeadAngleRequest, callback));
  }
}


void SDKComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  // TODO
}

void SDKComponent::HandleMessage(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  switch(event.GetData().GetTag()){
    // Receives a message that external SDK wants an SDK behavior to be activated.
    case external_interface::GatewayWrapperTag::kControlRequest:
      LOG_INFO("SDKComponent.HandleMessageRequest", "SDK requested control");
      _sdkWantsControl = true;

      if (_sdkBehaviorActivated) {
        LOG_INFO("SDKComponent.HandleMessageBehaviorActivated", "SDK already has control");
        // SDK wants control and and the SDK Behavior is already running. Send response that SDK behavior is activated.
        DispatchSDKActivationResult(_sdkBehaviorActivated);
        return;
      }
      break;

    case external_interface::GatewayWrapperTag::kControlRelease:
      LOG_INFO("SDKComponent.HandleMessageRelease", "Releasing SDK control");
      _sdkWantsControl = false;
      break;

    default:
      _robot->GetRobotEventHandler().HandleMessage(event);
      break;
  }
}

bool SDKComponent::SDKWantsControl()
{
  // TODO What slot does the SDK want to run at? Currently only requesting at one slot, SDK0.
  return _sdkWantsControl;
}

void SDKComponent::SDKBehaviorActivation(bool enabled)
{
  _sdkBehaviorActivated = enabled;
  DispatchSDKActivationResult(_sdkBehaviorActivated);
}

void SDKComponent::DispatchSDKActivationResult(bool enabled) {
  auto* gi = _robot->GetGatewayInterface();
  if (enabled) {
    //TODO: better naming, more readable, and logging
    auto* msg = new external_interface::BehaviorControlResponse(new external_interface::ControlGrantedResponse());
    gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
  }
  else {
    auto* msg = new external_interface::BehaviorControlResponse(new external_interface::ControlLostResponse());
    gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
  }
}

void SDKComponent::OnActionCompleted(ExternalInterface::RobotCompletedAction msg)
{
  auto* gi = _robot->GetGatewayInterface();
  if (gi == nullptr) return;

  switch((RobotActionType)msg.actionType)
  {
    case RobotActionType::TURN_IN_PLACE:
    {
      auto* response = new external_interface::TurnInPlaceResponse;
      response->set_result(static_cast<external_interface::ActionResult>(msg.result));
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    case RobotActionType::DRIVE_STRAIGHT:
    {
      auto* response = new external_interface::DriveStraightResponse;
      response->set_result(static_cast<external_interface::ActionResult>(msg.result));
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    case RobotActionType::MOVE_HEAD_TO_ANGLE:
    {
      auto* response = new external_interface::SetHeadAngleResponse;
      response->set_result(static_cast<external_interface::ActionResult>(msg.result));
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    case RobotActionType::MOVE_LIFT_TO_HEIGHT:
    {
      auto* response = new external_interface::SetLiftHeightResponse;
      response->set_result(static_cast<external_interface::ActionResult>(msg.result));
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

    case RobotActionType::DRIVE_TO_POSE:
    {
      auto* response = new external_interface::GoToPoseResponse;
      response->set_result(static_cast<external_interface::ActionResult>(msg.result));
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    case RobotActionType::ALIGN_WITH_OBJECT:
    {
      auto* response = new external_interface::DockWithCubeResponse;
      response->set_result(static_cast<external_interface::ActionResult>(msg.result));
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    default:
    {
      PRINT_NAMED_WARNING("SDKComponent.OnActionCompleted.NoMatch", "No match for action tag so no response sent: [Tag=%d]", msg.idTag);
      return;
    }
  }
}

} // namespace Vector
} // namespace Anki
