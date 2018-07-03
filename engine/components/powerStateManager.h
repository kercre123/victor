/**
 * File: powerStateManager.h
 *
 * Author: Brad Neuman
 * Created: 2018-06-27
 *
 * Description: Central engine component to manage power states (i.e. "power save mode")
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_PowerStateManager_H__
#define __Engine_Components_PowerStateManager_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "engine/contextWrapper.h"

#include <set>

namespace Anki {
namespace Cozmo {

class CozmoContext;

enum class PowerSaveSetting {
  CalmMode,
  Camera,
  LCDBacklight,
  ThrottleCPU,
};

class PowerStateManager : public IDependencyManagedComponent<RobotComponentID>,
                          public UnreliableComponent<BCComponentID>,
                          private Anki::Util::noncopyable
{
public:
  PowerStateManager();

  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  }
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;

  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::AIComponent);
  }
  virtual void AdditionalUpdateAccessibleComponents(RobotCompIDSet& comps) const override {
    comps.insert(RobotComponentID::Vision);
    comps.insert(RobotComponentID::MicComponent);
  }

  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;


  // Request the robot enter power save mode. If any requests are active, this component will attempt to enter
  // power save. The requester string should be unique and is useful for debugging
  void RequestPowerSaveMode(const std::string& requester);

  // Remove a specific power save request, and return true if one was found. Once no requests remain, this
  // component will attempt to leave power save mode
  bool RemovePowerSaveModeRequest(const std::string& requester);

  bool InPowerSaveMode() const { return _inPowerSaveMode; }

  // NOTE: In an ideal system, we'd work the opposite way, where specific behaviors or pieces of code could
  // request a higher power mode, and the _default_ would be power save. This would potentially allow better
  // power saving, but also be harder to debug and have systems understand what to do and not do in power save
  // mode, so for now it's "opt-in" instead of "opt-out"

  // prevent hiding warnings (use the robot component versions for real)
  using UnreliableComponent<BCComponentID>::GetInitDependencies;
  using UnreliableComponent<BCComponentID>::InitDependent;
  using UnreliableComponent<BCComponentID>::GetUpdateDependencies;
  using UnreliableComponent<BCComponentID>::UpdateDependent;
  using UnreliableComponent<BCComponentID>::AdditionalUpdateAccessibleComponents;

private:

  std::multiset<std::string> _powerSaveRequests;
  bool _inPowerSaveMode = false;

  void EnterPowerSave(const RobotCompMap& components);
  void ExitPowerSave(const RobotCompMap& components);

  void TogglePowerSaveSetting( const RobotCompMap& components, PowerSaveSetting setting, bool enabled );

  const CozmoContext* _context = nullptr;

  std::set<PowerSaveSetting> _enabledSettings;

  enum class CameraState {
    Running,
    ShouldDelete,
    Deleted
  };

  CameraState _cameraState = CameraState::Running;

  bool _cpuThrottleLow = true;
};

}
}

#endif
