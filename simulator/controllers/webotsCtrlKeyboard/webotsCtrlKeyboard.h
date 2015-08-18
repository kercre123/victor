/*
 * File:          webotsCtrlKeyboard.cpp
 * Date:
 * Description:
 * Author:
 * Modifications:
 */


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

}; // class WebotsKeyboardController
} // namespace Cozmo
} // namespace Anki


