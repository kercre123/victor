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

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

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


void SDKComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
{
  // TODO It's preferred, where possible, to use dependentComps rather than caching robot
  // directly (makes it much easier to write unit tests)
  _robot = robot;
  // Subscribe to messages
  if( _robot->HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this,  _signalHandles);

    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<ExternalInterface::MessageGameToEngineTag::SDKActivationRequest>();
  }
}


void SDKComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  // TODO
}

// Receives a message that external SDK wants an SDK behavior to be activated.
template<>
void SDKComponent::HandleMessage(const ExternalInterface::SDKActivationRequest& msg)
{
  if (msg.enable) {
    _sdkWantsControl = true;

    if (_sdkBehaviorActivated) {
      // SDK wants control and and the SDK Behavior is already running. Send response that SDK behavior is activated.
      DispatchSDKActivationResult(_sdkBehaviorActivated);
      return;
    }
  }
  else {
    _sdkWantsControl = false;
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
  // Communicate to SDK that SDK behavior has been activated or deactivated.
  // When activated, SDK can control the robot.
  ExternalInterface::SDKActivationResult sar;
  sar.slot = BehaviorSlot::SDK_HIGH_PRIORITY; // TODO Allow multiple levels to be used; don't hardcode. Also, this correlates to SDK0. Rename SDK0 to SDK_HIGH_PRIORITY or similar?
  sar.enabled = enabled;
  _robot->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(std::move(sar)));  
}

} // namespace Cozmo
} // namespace Anki
