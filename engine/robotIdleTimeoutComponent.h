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
#include "engine/entity.h"
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
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  // Maintain the chain of initializations currently in robot - it might be possible to
  // change the order of initialization down the line, but be sure to check for ripple effects
  // when changing this function
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::RobotToEngineImplMessaging);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  void Update(float currentTime_s);
  
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
