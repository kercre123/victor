/*
 * File:          webotsCtrlKeyboard.cpp
 * Date:
 * Description:
 * Author:
 * Modifications:
 */

#include <opencv2/imgproc/imgproc.hpp>
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/ledTypes.h"
#include "anki/cozmo/shared/activeBlockTypes.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/vision/basestation/image.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/messaging/shared/TcpClient.h"
#include "anki/cozmo/game/comms/gameMessageHandler.h"
#include "anki/cozmo/game/comms/gameComms.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/block.h"
#include "clad/types/actionTypes.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <webots/Supervisor.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Display.hpp>
#include <webots/GPS.hpp>
#include <webots/Compass.hpp>


namespace Anki {
namespace Cozmo {

namespace WebotsKeyboardController {

// Forward declarations
void ProcessKeystroke(void);
void ProcessJoystick(void);
void PrintHelp(void);
void SendMessage(const ExternalInterface::MessageGameToEngine& msg);
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
void SendSetRobotImageSendMode(u8 mode);
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




} // namespace WebotsKeyboardController
} // namespace Cozmo
} // namespace Anki


