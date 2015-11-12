/*
 * File:          uiGameController.h
 * Date:
 * Description:   Any UI/Game to be run as a Webots controller should be derived from this class.
 * Author:
 * Modifications:
 */

#ifndef __UI_GAME_CONTROLLER_H__
#define __UI_GAME_CONTROLLER_H__

#include "anki/types.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/imageTypes.h"
#include "clad/types/robotTestModes.h"
#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/visionModes.h"
#include <webots/Supervisor.hpp>
#include <unordered_set>

namespace Anki {
  
  // Forward declaration:
  namespace Util {
    namespace Data {
      class DataPlatform;
    }
  }
  
namespace Cozmo {

class UiGameController {

public:
  typedef struct {
    ObjectFamily family;
    ObjectType   type;
    s32 id;
    f32 area;
    bool isActive;
    
    void Reset() {
      family = ObjectFamily::Unknown;
      type = ObjectType::Unknown;
      id = -1;
      area = 0;
      isActive = false;
    }
  } ObservedObject;
  
  
  
  UiGameController(s32 step_time_ms);
  ~UiGameController();
  
  void Init();
  s32 Update();
  
  void SetDataPlatform(Util::Data::DataPlatform* dataPlatform);
  Util::Data::DataPlatform* GetDataPlatform();
  
  void QuitWebots(s32 status);
  void QuitController(s32 status);
  
  void UpdateVizOrigin();
  
protected:
  
  virtual void InitInternal() {}
  virtual s32 UpdateInternal() = 0;

  
  // TODO: These default handlers and senders should be CLAD-generated!
  
  // Message handlers
  virtual void HandleRobotStateUpdate(ExternalInterface::RobotState const& msg){};
  virtual void HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg){};
  virtual void HandleRobotObservedFace(ExternalInterface::RobotObservedFace const& msg){};
  virtual void HandleRobotObservedNothing(ExternalInterface::RobotObservedNothing const& msg){};
  virtual void HandleRobotDeletedObject(ExternalInterface::RobotDeletedObject const& msg){};
  virtual void HandleRobotConnection(const ExternalInterface::RobotAvailable& msgIn){};
  virtual void HandleUiDeviceConnection(const ExternalInterface::UiDeviceAvailable& msgIn){};
  virtual void HandleRobotConnected(ExternalInterface::RobotConnected const &msg){};
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg){};
  virtual void HandleImageChunk(ImageChunk const& msg){};
  virtual void HandleActiveObjectMoved(ObjectMoved const& msg){};
  virtual void HandleActiveObjectStoppedMoving(ObjectStoppedMoving const& msg){};
  virtual void HandleActiveObjectTapped(ObjectTapped const& msg){};
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
  void SendImageRequest(ImageSendMode mode, u8 robotID);
  void SendSetRobotImageSendMode(ImageSendMode mode, ImageResolution resolution);
  void SendSaveImages(SaveMode_t mode, bool alsoSaveState=false);
  void SendEnableDisplay(bool on);
  void SendExecutePathToPose(const Pose3d& p,
                             PathMotionProfile motionProf,
                             const bool useManualSpeed);
  void SendGotoObject(const s32 objectID,
                      const f32 distFromObjectOrigin_mm,
                      PathMotionProfile motionProf,
                      const bool useManualSpeed = false);
  
  void SendAlignWithObject(const s32 objectID,
                           const f32 distFromMarker_mm,
                           PathMotionProfile motionProf,
                           const bool usePreDockPose,
                           const bool useApproachAngle = false,
                           const f32 approachAngle_rad = false,
                           const bool useManualSpeed = false);
  
  void SendPlaceObjectOnGroundSequence(const Pose3d& p,
                                       PathMotionProfile motionProf,
                                       const bool useExactRotation = false,
                                       const bool useManualSpeed = false);
  
  void SendPickupObject(const s32 objectID,
                        PathMotionProfile motionProf,
                        const bool usePreDockPose,
                        const bool useApproachAngle = false,
                        const f32 approachAngle_rad = false,
                        const bool useManualSpeed = false);
  
  void SendPickupSelectedObject(PathMotionProfile motionProf,
                                const bool usePreDockPose,
                                const bool useApproachAngle,
                                const f32 approachAngle_rad,
                                const bool useManualSpeed = false);
  
  void SendPlaceOnObject(const s32 objectID,
                         PathMotionProfile motionProf,
                         const bool usePreDockPose,
                         const bool useApproachAngle = false,
                         const f32 approachAngle_rad = 0,
                         const bool useManualSpeed = false);
  
  void SendPlaceOnSelectedObject(PathMotionProfile motionProf,
                                 const bool usePreDockPose,
                                 const bool useApproachAngle = false,
                                 const f32 approachAngle_rad = 0,
                                 const bool useManualSpeed = false);

  void SendPlaceRelObject(const s32 objectID,
                          PathMotionProfile motionProf,
                          const bool usePreDockPose,
                          const f32 placementOffsetX_mm,
                          const bool useApproachAngle = false,
                          const f32 approachAngle_rad = 0,
                          const bool useManualSpeed = false);
  
  void SendPlaceRelSelectedObject(PathMotionProfile motionProf,
                                  const bool usePreDockPose,
                                  const f32 placementOffsetX_mm,
                                  const bool useApproachAngle = false,
                                  const f32 approachAngle_rad = 0,
                                  const bool useManualSpeed = false);

  void SendRollObject(const s32 objectID,
                      PathMotionProfile motionProf,
                      const bool usePreDockPose,
                      const bool useApproachAngle = false,
                      const f32 approachAngle_rad = 0,
                      const bool useManualSpeed = false);
  
  void SendRollSelectedObject(PathMotionProfile motionProf,
                              const bool usePreDockPose,
                              const bool useApproachAngle = false,
                              const f32 approachAngle_rad = 0,
                              const bool useManualSpeed = false);
  
  void SendTraverseSelectedObject(PathMotionProfile motionProf,
                                  const bool usePreDockPose,
                                  const bool useManualSpeed);
  void SendExecuteTestPlan();
  void SendClearAllBlocks();
  void SendClearAllObjects();
  void SendSelectNextObject();
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
  void SendEnableVisionMode(VisionMode mode, bool enable);
  void SendForceAddRobot();
  void SendSetIdleAnimation(const std::string &animName);
  void SendQueuePlayAnimAction(const std::string &animName, u32 numLoops, QueueActionPosition pos);
  void SendCancelAction();
  

  // ====== Accessors =====
  s32 GetStepTimeMS() const;
  webots::Supervisor* GetSupervisor() const;

  // Robot state message convenience functions
  const Pose3d& GetRobotPose() const;
  const Pose3d& GetRobotPoseActual() const;
  f32           GetRobotHeadAngle_rad() const;
  f32           GetLiftHeight_mm() const;
  void          GetWheelSpeeds_mmps(f32& left, f32& right) const;
  s32           GetCarryingObjectID() const;
  s32           GetCarryingObjectOnTopID() const;
  bool          IsRobotStatus(RobotStatusFlag mask) const;
  
  std::vector<s32> GetAllObjectIDs() const;
  std::vector<s32> GetAllObjectIDsByFamily(ObjectFamily family) const;
  std::vector<s32> GetAllObjectIDsByFamilyAndType(ObjectFamily family, ObjectType type) const;
  Result           GetObjectFamily(s32 objectID, ObjectFamily& family) const;
  Result           GetObjectType(s32 objectID, ObjectType& type) const;
  Result           GetObjectPose(s32 objectID, Pose3d& pose) const;
  
  u32              GetNumObjectsInFamily(ObjectFamily family) const;
  u32              GetNumObjectsInFamilyAndType(ObjectFamily family, ObjectType type) const;
  u32              GetNumObjects() const;
  void             ClearAllKnownObjects();
  
  const std::map<s32, Pose3d>& GetObjectPoseMap();
  
  const ObservedObject& GetCurrentlyObservedObject() const;

  
  const std::unordered_set<std::string>& GetAvailableAnimations() const;
  u32 GetNumAvailableAnimations() const;
  bool IsAvailableAnimation(std::string anim) const;
  BehaviorType GetBehaviorType(const std::string& behaviorName) const;

  
  // Actually move objects in the simulated world
  void SetActualRobotPose(const Pose3d& newPose);
  void SetActualObjectPose(const std::string& name, const Pose3d& newPose);
  
private:
  void HandleRobotStateUpdateBase(ExternalInterface::RobotState const& msg);
  void HandleRobotObservedObjectBase(ExternalInterface::RobotObservedObject const& msg);
  void HandleRobotObservedFaceBase(ExternalInterface::RobotObservedFace const& msg);
  void HandleRobotObservedNothingBase(ExternalInterface::RobotObservedNothing const& msg);
  void HandleRobotDeletedObjectBase(ExternalInterface::RobotDeletedObject const& msg);
  void HandleRobotConnectionBase(ExternalInterface::RobotAvailable const& msgIn);
  void HandleUiDeviceConnectionBase(ExternalInterface::UiDeviceAvailable const& msgIn);
  void HandleRobotConnectedBase(ExternalInterface::RobotConnected const &msg);
  void HandleRobotCompletedActionBase(ExternalInterface::RobotCompletedAction const& msg);
  void HandleImageChunkBase(ImageChunk const& msg);
  void HandleActiveObjectMovedBase(ObjectMoved const& msg);
  void HandleActiveObjectStoppedMovingBase(ObjectStoppedMoving const& msg);
  void HandleActiveObjectTappedBase(ObjectTapped const& msg);
  void HandleAnimationAvailableBase(ExternalInterface::AnimationAvailable const& msg);
  
  void UpdateActualObjectPoses();
  
}; // class UiGameController
  
  
  
} // namespace Cozmo
} // namespace Anki


#endif // __UI_GAME_CONTROLLER_H__
