/**
 * File: trackFaceAction.cpp
 *
 * Author: Andrew Stein
 * Date:   12/11/2015
 *
 * Description: Defines an action for tracking (human) faces, derived from ITrackAction
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/actions/trackFaceAction.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageEngineToGame.h"

#define DEBUG_TRACKING_ACTIONS 0

namespace Anki {
namespace Cozmo {
  
static const char * const kLogChannelName = "Actions";
  
TrackFaceAction::TrackFaceAction(Robot& robot, FaceID faceID)
  : ITrackAction(robot,
                 "TrackFace",
                 RobotActionType::TRACK_FACE)
  , _faceID(faceID)
{
   SetName("TrackFace" + std::to_string(_faceID));
}

TrackFaceAction::~TrackFaceAction()
{
  _robot.GetMoveComponent().UnSetTrackToFace();
}


ActionResult TrackFaceAction::InitInternal()
{
  if(false == _robot.HasExternalInterface()) {
    PRINT_NAMED_ERROR("TrackFaceAction.InitInternal.NoExternalInterface",
                      "Robot must have an external interface so action can "
                      "subscribe to face changed ID events.");
    return ActionResult::ABORT;
  }
  
  using namespace ExternalInterface;
  auto HandleFaceChangedID = [this](const AnkiEvent<MessageEngineToGame>& event)
  {
    auto & msg = event.GetData().Get_RobotChangedObservedFaceID();
    if(msg.oldID == _faceID)
    {
      PRINT_CH_INFO(kLogChannelName, "TrackFaceAction.HandleFaceChangedID",
                    "Updating tracked face ID from %d to %d",
                    msg.oldID, msg.newID);
      
      _faceID = msg.newID;
    }
  };
  
  _signalHandle = _robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotChangedObservedFaceID, HandleFaceChangedID);
  
  _robot.GetMoveComponent().SetTrackToFace(_faceID);
  _lastFaceUpdate = 0;
  
  return ActionResult::SUCCESS;
} // InitInternal()

void TrackFaceAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
{
  TrackFaceCompleted completion;
  completion.faceID = static_cast<s32>(_faceID);
  completionUnion.Set_trackFaceCompleted(std::move(completion));
}
  
ITrackAction::UpdateResult TrackFaceAction::UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm)
{
  const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_faceID);
  distance_mm = 0.f;
  
  if(nullptr == face) {
    // No such face
    PRINT_CH_INFO(kLogChannelName, "TrackFaceAction.UpdateTracking.BadFaceID", "No face %d in FaceWorld", _faceID);
    return UpdateResult::NoNewInfo;
  }
  
  // Only update pose if we've actually observed the face again since last update
  if(face->GetTimeStamp() <= _lastFaceUpdate) {
    return UpdateResult::NoNewInfo;
  }
  _lastFaceUpdate = face->GetTimeStamp();
  
  Pose3d headPoseWrtRobot;
  if(false == face->GetHeadPose().GetWithRespectTo(_robot.GetPose(), headPoseWrtRobot)) {
    PRINT_NAMED_ERROR("TrackFaceAction.UpdateTracking.PoseOriginError",
                      "Could not get pose of face w.r.t. robot.");
    return UpdateResult::NoNewInfo;
  }
  
  const f32 xDist = headPoseWrtRobot.GetTranslation().x();
  const f32 yDist = headPoseWrtRobot.GetTranslation().y();
  
  // NOTE: This isn't perfectly accurate since it doesn't take into account the
  // the head angle and is simply using the neck joint (which should also
  // probably be queried from the robot instead of using the constant here)
  const f32 zDist = headPoseWrtRobot.GetTranslation().z() - NECK_JOINT_POSITION[2];

  if(DEBUG_TRACKING_ACTIONS)
  {
    PRINT_NAMED_INFO("TrackFaceAction.UpdateTracking.HeadPose",
                     "Translation w.r.t. robot = (%.1f, %.1f, %.1f) [t=%d]",
                     xDist, yDist, zDist, face->GetTimeStamp());
  }
  
  const f32 xyDistSq = xDist*xDist + yDist*yDist;
  if (xyDistSq <= 0.f)
  {
    DEV_ASSERT(false, "TrackFaceAction.UpdateTracking.ZeroDistance");
    return UpdateResult::NoNewInfo;
  }
  
  absTiltAngle = std::atan(zDist/std::sqrt(xyDistSq));
  absPanAngle  = std::atan2(yDist, xDist) + _robot.GetPose().GetRotation().GetAngleAroundZaxis();

  return UpdateResult::NewInfo;

} // UpdateTracking()
  
} // namespace Cozmo
} // namespace Anki
