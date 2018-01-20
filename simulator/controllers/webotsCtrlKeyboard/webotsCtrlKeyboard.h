/*
 * File:          webotsCtrlKeyboard.cpp
 * Date:
 * Description:
 * Author:
 * Modifications:
 */

#ifndef __webotsCtrlKeyboard_H_
#define __webotsCtrlKeyboard_H_

#include "simulator/game/uiGameController.h"

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
  
  Pose3d GetGoalMarkerPose();
  
  virtual void InitInternal() override;
  virtual s32 UpdateInternal() override;

  virtual void HandleImageChunk(const ImageChunk& msg) override;
  virtual void HandleRobotObservedObject(const ExternalInterface::RobotObservedObject& msg) override;
  virtual void HandleRobotObservedFace(const ExternalInterface::RobotObservedFace& msg) override;
  virtual void HandleRobotObservedPet(const ExternalInterface::RobotObservedPet& msg) override;
  virtual void HandleDebugString(const ExternalInterface::DebugString& msg) override;
  virtual void HandleNVStorageOpResult(const ExternalInterface::NVStorageOpResult& msg) override;
  virtual void HandleFaceEnrollmentCompleted(const ExternalInterface::FaceEnrollmentCompleted& msg) override;
  virtual void HandleLoadedKnownFace(const Vision::LoadedKnownFace& msg) override;
  virtual void HandleEngineErrorCode(const ExternalInterface::EngineErrorCodeMessage& msg) override;
  virtual void HandleRobotConnected(const ExternalInterface::RobotConnectionResponse& msg) override;
  
private:

  bool _shouldQuit = false;
  
}; // class WebotsKeyboardController
} // namespace Cozmo
} // namespace Anki

#endif  // __webotsCtrlKeyboard_H_
