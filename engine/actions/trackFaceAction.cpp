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

#include "engine/actions/trackFaceAction.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "coretech/common/engine/utils/timer.h"

#define DEBUG_TRACKING_ACTIONS 0

namespace Anki {
namespace Cozmo {
  
static const char * const kLogChannelName = "Actions";
  
TrackFaceAction::TrackFaceAction(FaceID rawFaceID)
  : ITrackAction("TrackFace",
                 RobotActionType::TRACK_FACE)
  , _tmpFaceID(rawFaceID)
{
  SetName("TrackFace" + std::to_string(rawFaceID));
}

TrackFaceAction::TrackFaceAction(SmartFaceID faceID)
  : ITrackAction("TrackFace",
                 RobotActionType::TRACK_FACE)
  , _faceID(faceID)
{
  SetName("TrackFace" + _faceID.GetDebugStr());
}

TrackFaceAction::~TrackFaceAction()
{
}

void TrackFaceAction::OnRobotSet()
{
  if( !_faceID.IsValid() ) {
    _faceID = GetRobot().GetFaceWorld().GetSmartFaceID(_tmpFaceID);
  }
}

void TrackFaceAction::GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const
{
  requests.insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
}

ActionResult TrackFaceAction::InitInternal()
{  
  _lastFaceUpdate = 0;
  
  return ActionResult::SUCCESS;
} // InitInternal()

void TrackFaceAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
{
  TrackFaceCompleted completion;
  completion.faceID = static_cast<s32>(_faceID.GetID());
  completionUnion.Set_trackFaceCompleted(std::move(completion));
}
  
ITrackAction::UpdateResult TrackFaceAction::UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm)
{
  const Vision::TrackedFace* face = GetRobot().GetFaceWorld().GetFace(_faceID);
  distance_mm = 0.f;
  
  if(nullptr == face) {
    // No such face
    PRINT_CH_INFO(kLogChannelName, "TrackFaceAction.UpdateTracking.BadFaceID",
                  "No face %s in FaceWorld",
                  _faceID.GetDebugStr().c_str());
    return UpdateResult::NoNewInfo;
  }
  
  // Only update pose if we've actually observed the face again since last update
  if(face->GetTimeStamp() <= _lastFaceUpdate) {
    return UpdateResult::NoNewInfo;
  }
  _lastFaceUpdate = face->GetTimeStamp();
  
  Pose3d headPoseWrtRobot;
  if(false == face->GetHeadPose().GetWithRespectTo(GetRobot().GetPose(), headPoseWrtRobot)) {
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
  absPanAngle  = std::atan2(yDist, xDist) + GetRobot().GetPose().GetRotation().GetAngleAroundZaxis();

  return UpdateResult::NewInfo;

} // UpdateTracking()

bool TrackFaceAction::ContinueCriteriaMet(const f32 currentTime_sec)
{
  if (Util::IsFltGTZero(_eyeContactCriteria.noEyeContactTimeout_sec))
  {
    // TODO it would ideal to make sure we only use eye contact from the
    // face we're tracking VIC-5557
    const bool eyeContact = GetRobot().GetFaceWorld().IsMakingEyeContact(_eyeContactCriteria.eyeContactWithinLast_ms);
    if (eyeContact)
    {
      _eyeContactCriteria.timeOfLastEyeContact_sec = currentTime_sec;
      return true;
    }
    else if (currentTime_sec - _eyeContactCriteria.timeOfLastEyeContact_sec <= _eyeContactCriteria.noEyeContactTimeout_sec)
    {
      return true;
    }
  }
  return false;
}

void TrackFaceAction::SetStopCriteriaWithEyeContactOverride(const f32 minTimeToTrack_sec, const f32 noEyeContactTimeout_sec,
                                                            const TimeStamp_t eyeContactWithinLast_ms)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetStopCriteria.ActionAlreadyStarted");
  const auto currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _stopCriteria.earliestStoppingTime_sec = currentTime_sec + minTimeToTrack_sec;

  _eyeContactCriteria.noEyeContactTimeout_sec = noEyeContactTimeout_sec;
  _eyeContactCriteria.eyeContactWithinLast_ms = eyeContactWithinLast_ms;
}
  
} // namespace Cozmo
} // namespace Anki
