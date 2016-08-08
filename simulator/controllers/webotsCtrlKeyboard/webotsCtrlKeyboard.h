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

  // Called before WaitOnKeyboardToConnect (and also before Init), sets up the basics needed for
  // WaitOnKeyboardToConnect, including enabling the keyboard
  void PreInit();
  
  // if the corresponding proto field is set to true, this function will wait until the user presses
  // Shift+enter to return.This can be used to allow unity to connect. If we notice another connection
  // attempt, we will exit the keyboard controller. This is called before Init
  void WaitOnKeyboardToConnect();

protected:
  void ProcessKeystroke();
  void PrintHelp();
  
  void TestLightCube();
  
  virtual void InitInternal() override;
  virtual s32 UpdateInternal() override;

  virtual void HandleImageChunk(ImageChunk const& msg) override;
  virtual void HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg) override;
  virtual void HandleRobotObservedFace(ExternalInterface::RobotObservedFace const& msg) override;
  virtual void HandleDebugString(ExternalInterface::DebugString const& msg) override;
  virtual void HandleNVStorageData(const ExternalInterface::NVStorageData &msg) override;
  virtual void HandleNVStorageOpResult(const ExternalInterface::NVStorageOpResult &msg) override;
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg) override;
  virtual void HandleLoadedKnownFace(Vision::LoadedKnownFace const& msg) override;
  
private:

  bool _shouldQuit = false;
  
}; // class WebotsKeyboardController
} // namespace Cozmo
} // namespace Anki

#endif  // __webotsCtrlKeyboard_H_
