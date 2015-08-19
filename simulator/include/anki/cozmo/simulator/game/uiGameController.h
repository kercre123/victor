/*
 * File:          uiGameController.h
 * Date:
 * Description:   Any UI/Game to be run as a Webots controller should be derived from this class.
 * Author:
 * Modifications:
 */

#ifndef __UI_GAME_CONTROLLER_H__
#define __UI_GAME_CONTROLLER_H__

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/data/dataPlatform.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include <webots/Supervisor.hpp>


namespace Anki {
namespace Cozmo {

class UiGameController {

public:
  typedef struct {
    s32 family;
    s32 type;
    s32 id;
    f32 area;
    bool isActive;
    
    void Reset() {
      family = -1;
      type = -1;
      id = -1;
      area = 0;
      isActive = false;
    }
  } ObservedObject;
  
  
  
  UiGameController(s32 step_time_ms);
  ~UiGameController();
  
  void Init();
  s32 Update();
  
  void SetDataPlatform(Data::DataPlatform* dataPlatform);
  Data::DataPlatform* GetDataPlatform();
  
  void QuitWebots(s32 status);
  void QuitController(s32 status);
  
protected:
  
  virtual void InitInternal() {}
  virtual s32 UpdateInternal() = 0;

  
  // Message handlers
  virtual void HandleRobotStateUpdate(ExternalInterface::RobotState const& msg){};
  virtual void HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg){};
  virtual void HandleRobotObservedNothing(ExternalInterface::RobotObservedNothing const& msg){};
  virtual void HandleRobotDeletedObject(ExternalInterface::RobotDeletedObject const& msg){};
  virtual void HandleRobotConnection(const ExternalInterface::RobotAvailable& msgIn){};
  virtual void HandleUiDeviceConnection(const ExternalInterface::UiDeviceAvailable& msgIn){};
  virtual void HandleRobotConnected(ExternalInterface::RobotConnected const &msg){};
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg){};
  virtual void HandleImageChunk(ExternalInterface::ImageChunk const& msg){};
  virtual void HandleActiveObjectMoved(ExternalInterface::ActiveObjectMoved const& msg){};
  virtual void HandleActiveObjectStoppedMoving(ExternalInterface::ActiveObjectStoppedMoving const& msg){};
  virtual void HandleActiveObjectTapped(ExternalInterface::ActiveObjectTapped const& msg){};
  virtual void HandleAnimationAvailable(ExternalInterface::AnimationAvailable const& msg){};
  
  
  // Message senders
  void SendMessage(const ExternalInterface::MessageGameToEngine& msg);
  void SendPing();
  void SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps);
  void SendTurnInPlace(const f32 angle_rad);
  void SendTurnInPlaceAtSpeed(const f32 speed_rad_per_sec, const f32 accel_rad_per_sec2);
  void SendMoveHead(const f32 speed_rad_per_sec);
  void SendMoveLift(const f32 speed_rad_per_sec);
  void SendMoveHeadToAngle(const f32 rad, const f32 speed, const f32 accel, const f32 duration_sec = 0.f);
  void SendMoveLiftToHeight(const f32 mm, const f32 speed, const f32 accel, const f32 duration_sec = 0.f);
  void SendTapBlockOnGround(const u8 numTaps);
  void SendStopAllMotors();
  void SendImageRequest(u8 mode, u8 robotID);
  void SendSetRobotImageSendMode(u8 mode, u8 resolution);
  void SendSaveImages(SaveMode_t mode, bool alsoSaveState=false);
  void SendEnableDisplay(bool on);
  void SendSetHeadlights(u8 intensity);
  void SendExecutePathToPose(const Pose3d& p, const bool useManualSpeed);
  void SendPlaceObjectOnGroundSequence(const Pose3d& p, const bool useManualSpeed);
  void SendPickAndPlaceObject(const s32 objectID, const bool usePreDockPose, const bool useManualSpeed);
  void SendPickAndPlaceSelectedObject(const bool usePreDockPose, const bool useManualSpeed);
  void SendRollObject(const s32 objectID, const bool usePreDockPose, const bool useManualSpeed);
  void SendRollSelectedObject(const bool usePreDockPose, const bool useManualSpeed);
  void SendTraverseSelectedObject(const bool usePreDockPose, const bool useManualSpeed);
  void SendExecuteTestPlan();
  void SendClearAllBlocks();
  void SendClearAllObjects();
  void SendSelectNextObject();
  void SendExecuteBehavior(BehaviorManager::Mode mode);
  void SendSetNextBehaviorState(BehaviorManager::BehaviorState nextState);
  void SendAbortPath();
  void SendAbortAll();
  void SendDrawPoseMarker(const Pose3d& p);
  void SendErasePoseMarker();
  void SendWheelControllerGains(const f32 kpLeft, const f32 kiLeft, const f32 maxErrorSumLeft,
    const f32 kpRight, const f32 kiRight, const f32 maxErrorSumRight);
  void SendHeadControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxErrorSum);
  void SendLiftControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxErrorSum);
  void SendSteeringControllerGains(const f32 k1, const f32 k2);
  void SendSetRobotVolume(const f32 volume);
  void SendStartTestMode(TestMode mode, s32 p1 = 0, s32 p2 = 0, s32 p3 = 0);
  void SendIMURequest(u32 length_ms);
  void SendAnimation(const char* animName, u32 numLoops);
  void SendReplayLastAnimation();
  void SendReadAnimationFile();
  void SendStartFaceTracking(u8 timeout_sec);
  void SendStopFaceTracking();
  void SendVisionSystemParams();
  void SendFaceDetectParams();
  void SendForceAddRobot();
  void SendSetIdleAnimation(const std::string &animName);
  void SendQueuePlayAnimAction(const std::string &animName, u32 numLoops, QueueActionPosition pos);
  void SendCancelAction();
  

  // Accessors
  s32 GetStepTimeMS();
  webots::Supervisor* GetSupervisor();
  
  const Pose3d& GetRobotPose();
  const Pose3d& GetRobotPoseActual();

  const ObservedObject& GetCurrentlyObservedObject();

  
private:
  void HandleRobotStateUpdateBase(ExternalInterface::RobotState const& msg);
  void HandleRobotObservedObjectBase(ExternalInterface::RobotObservedObject const& msg);
  void HandleRobotObservedNothingBase(ExternalInterface::RobotObservedNothing const& msg);
  void HandleRobotDeletedObjectBase(ExternalInterface::RobotDeletedObject const& msg);
  void HandleRobotConnectionBase(ExternalInterface::RobotAvailable const& msgIn);
  void HandleUiDeviceConnectionBase(ExternalInterface::UiDeviceAvailable const& msgIn);
  void HandleRobotConnectedBase(ExternalInterface::RobotConnected const &msg);
  void HandleRobotCompletedActionBase(ExternalInterface::RobotCompletedAction const& msg);
  void HandleImageChunkBase(ExternalInterface::ImageChunk const& msg);
  void HandleActiveObjectMovedBase(ExternalInterface::ActiveObjectMoved const& msg);
  void HandleActiveObjectStoppedMovingBase(ExternalInterface::ActiveObjectStoppedMoving const& msg);
  void HandleActiveObjectTappedBase(ExternalInterface::ActiveObjectTapped const& msg);
  void HandleAnimationAvailableBase(ExternalInterface::AnimationAvailable const& msg);
  
  void UpdateActualObjectPoses();
  
}; // class UiGameController
  
  
  
} // namespace Cozmo
} // namespace Anki


#endif // __UI_GAME_CONTROLLER_H__