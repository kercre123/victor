/**
* File: robotIdleTimeoutComponent
*
* Author: Lee Crippen
* Created: 8/9/2016
*
* Description: Handles the idle timeout functionality for the Robot. Listens for messages from Game
* for setting a timers that will trigger Cozmo going to sleep & turning off his screen, then cause Cozmo
* to disconnect from the engine entirely.
*
* Copyright: Anki, inc. 2016
*
*/
#ifndef __Anki_Cozmo_Basestation_RobotIdleTimeoutComponent_H_
#define __Anki_Cozmo_Basestation_RobotIdleTimeoutComponent_H_

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/entity.h"
#include "util/signals/signalHolder.h"

namespace Anki {
namespace Cozmo {
  
class Robot;
class IActionRunner;

class RobotIdleTimeoutComponent : public IDependencyManagedComponent<RobotComponentID>, protected Util::SignalHolder
{
public:
  RobotIdleTimeoutComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
  static IActionRunner* CreateGoToSleepAnimSequence();
  
  bool IdleTimeoutSet() const { return _disconnectTimeout_s > 0; }
  
private:
  Robot* _robot = nullptr;
  
  float _faceOffTimeout_s = -1.0f;
  float _disconnectTimeout_s = -1.0f;
  
}; // class RobotIdleTimeoutComponent

} // end namespace Cozmo
} // end namespace Anki


#endif //__Anki_Cozmo_Basestation_RobotIdleTimeoutComponent_H_
