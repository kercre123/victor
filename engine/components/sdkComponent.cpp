/**
 * File: sdkComponent
 *
 * Author: Michelle Sintov
 * Created: 05/25/2018
 *
 * Description: Component that serves as a mediator between external SDK requests and any instances of SDK behaviors,
 * such as SDK0.
 * 
 * The sdkComponent does the following:
 *     - The sdkComponent will know when there is an external sdk connection alive
 *     - The sdkComponent will receive a request from the external SDK to make the robot do something (which could be a
 *       behavior, action or low level motor control)
 *     - The SDK behavior will check the sdkComponent to see if the SDK wants to be activated
 *     - If the SDK behavior is activated, it will inform this component that the SDK has control
 *     - The sdkComponent informs the external SDK that it has control
 *     - Any other state changes from the SDK behavior (failed to get control, lost control, etc.) are also communicated
 *       through the sdkComponent.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/sdkComponent.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"

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

  // TODO
}

void SDKComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  // TODO
}

} // namespace Cozmo
} // namespace Anki
