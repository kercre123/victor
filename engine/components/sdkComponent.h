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
namespace Cozmo {

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
  
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

  // Event/Message handling
  template<typename T>
  void HandleMessage(const T& msg);
  
private:
  
  Robot* _robot = nullptr;  
  bool _sdkWantsControl = false;

  std::vector<::Signal::SmartHandle> _signalHandles;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Components_SDKComponent_H_