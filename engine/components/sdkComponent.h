/**
 * File: sdkComponent
 *
 * Author: Michelle Sintov
 * Created: 05/25/2018
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_SDKComponent_H_
#define __Engine_Components_SDKComponent_H_

#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Vector {

namespace external_interface {
  class GatewayWrapper;

  class PathMotionProfile;

  class GoToPoseRequest;
  class DockWithCubeRequest;
  class DriveStraightRequest;
  class TurnInPlaceRequest;
  class SetLiftHeightRequest;
  class SetHeadAngleRequest;
}

class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SDKComponent : public IDependencyManagedComponent<RobotComponentID>
{
public:

  SDKComponent();
  ~SDKComponent();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent functions

  virtual void GetInitDependencies( RobotCompIDSet& dependencies ) const override;
  virtual void GetUpdateDependencies( RobotCompIDSet& dependencies ) const override {};

  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

  // Event/Message handling
  void HandleMessage(const AnkiEvent<external_interface::GatewayWrapper>& event);

  bool SDKWantsControl();
  void SDKBehaviorActivation(bool enabled);

  void OnActionCompleted(ExternalInterface::RobotCompletedAction msg);

private:

  Robot* _robot = nullptr;  
  bool _sdkWantsControl = false;
  bool _sdkBehaviorActivated = false;

  std::vector<::Signal::SmartHandle> _signalHandles;

  void OnSendAudioModeRequest(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void DispatchSDKActivationResult(bool enabled);
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_Components_SDKComponent_H_