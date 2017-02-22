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

#include "util/signals/signalHolder.h"

namespace Anki {
namespace Cozmo {
  
class Robot;
class IActionRunner;

class RobotIdleTimeoutComponent : protected Util::SignalHolder
{
public:
  RobotIdleTimeoutComponent(Robot& robot);
  
  void Update(float currentTime_s);
  
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
  static IActionRunner* CreateGoToSleepAnimSequence(Robot& robot);
  
private:
  Robot& _robot;
  
  float _faceOffTimeout_s = -1.0f;
  float _disconnectTimeout_s = -1.0f;
  
}; // class RobotIdleTimeoutComponent

} // end namespace Cozmo
} // end namespace Anki


#endif //__Anki_Cozmo_Basestation_RobotIdleTimeoutComponent_H_
