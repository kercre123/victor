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


void SDKComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  // TODO It's preferred, where possible, to use dependentComponents rather than caching robot
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


template<>
void SDKComponent::HandleMessage(const ExternalInterface::SDKActivationRequest& msg)
{
  if (msg.enable) {
    _sdkWantsControl = true;
  }
  else {
    _sdkWantsControl = false;
  }
}


} // namespace Cozmo
} // namespace Anki
