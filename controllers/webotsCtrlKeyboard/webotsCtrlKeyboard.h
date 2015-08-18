/*
 * File:          webotsCtrlKeyboard.cpp
 * Date:
 * Description:
 * Author:
 * Modifications:
 */

#ifndef __webotsCtrlKeyboard_H_
#define __webotsCtrlKeyboard_H_

#include "anki/cozmo/simulator/game/uiGameController.h"

namespace Anki {
namespace Cozmo {

class WebotsKeyboardController : public UiGameController {
public:
  WebotsKeyboardController(s32 step_time_ms);

protected:
  void ProcessKeystroke();
  void ProcessJoystick();
  void PrintHelp();
  
  void TestLightCube();

  virtual void InitInternal() override;
  virtual s32 UpdateInternal() override;

  virtual void HandleImageChunk(ExternalInterface::ImageChunk const& msg) override;
  virtual void HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg) override;
  
}; // class WebotsKeyboardController
} // namespace Cozmo
} // namespace Anki

#endif  // __webotsCtrlKeyboard_H_
