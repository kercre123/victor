/**
 * File: behaviorReactToMotion.cpp
 *
 * Author: Andrew Stein
 * Created: 11/13/15
 *
 * Description: Behavior for following motion in the image
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorReactToMotion.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static std::vector<std::string> _animReactions = {
  "Demo_Motion_Reaction",
};

BehaviorReactToMotion::BehaviorReactToMotion(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  _name = "ReactToMotion";
  
  SubscribeToTags({MessageEngineToGameTag::RobotObservedMotion});
}

bool BehaviorReactToMotion::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  return true;
}

Result BehaviorReactToMotion::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  _interrupted = false;
  
  // Store whatever modes the vision system was using so we can restore once this
  // behavior completes
  // NOTE: if vision modes are enabled _while_ this behavior is running, they will be
  //       disabled when it finishes!
  
  _originalVisionModes = robot.GetVisionComponent().GetEnabledModes();
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingMotion, true);

  return Result::RESULT_OK;
}

IBehavior::Status BehaviorReactToMotion::UpdateInternal(Robot& robot, double currentTime_sec)
{
  if(_interrupted) {
    // Restore original vision modes
    robot.GetVisionComponent().SetModes(_originalVisionModes);
    return Status::Complete;
  } else {
    return Status::Running;
  }
}

Result BehaviorReactToMotion::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  return Result::RESULT_OK;
}

void BehaviorReactToMotion::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion:
    {
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      
      const Point2f motionCentroid(motionObserved.img_x, motionObserved.img_y);
      
      robot.GetMoodManager().AddToEmotions(EmotionType::Happy,   kEmotionChangeSmall,
                                           EmotionType::Excited, kEmotionChangeSmall,
                                           EmotionType::Calm,   -kEmotionChangeSmall, "MotionReact");
      
      const Vision::CameraCalibration& calibration = robot.GetVisionComponent().GetCameraCalibration();
      if(NEAR(motionCentroid.x(), 0.f, calibration.GetNcols()/10) &&
         NEAR(motionCentroid.y(), 0.f, calibration.GetNrows()/10))
      {
        // Move towards the motion since it's centered
        const f32 dist_mm = 20;
        Pose3d newPose(robot.GetPose());
        const f32 angleRad = newPose.GetRotation().GetAngleAroundZaxis().ToFloat();
        newPose.SetTranslation({newPose.GetTranslation().x() + dist_mm*std::cos(angleRad),
          newPose.GetTranslation().y() + dist_mm*std::sin(angleRad),
          newPose.GetTranslation().z()});
        PathMotionProfile motionProfile(DEFAULT_PATH_MOTION_PROFILE);
        motionProfile.speed_mmps *= 1.5f; // Drive forward a little faster than normal
        robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot,
                                             new DriveToPoseAction(newPose, motionProfile,
                                                                   false, false, 50, DEG_TO_RAD(45))
                                             );
        
      } else {
        // Turn to face the motion:
        // Convert image positions to desired angles (in absolute coordinates)
        RobotInterface::PanAndTilt msg;
        const f32 relHeadAngle_rad = std::atan(-motionCentroid.y() / calibration.GetFocalLength_y());
        msg.headTiltAngle_rad = robot.GetHeadAngle() + relHeadAngle_rad;
        const f32 relBodyPanAngle_rad = std::atan(-motionCentroid.x() / calibration.GetFocalLength_x());
        msg.bodyPanAngle_rad = (robot.GetPose().GetRotation().GetAngleAroundZaxis() + relBodyPanAngle_rad).ToFloat();
        
        robot.SendRobotMessage<RobotInterface::PanAndTilt>(std::move(msg));
      }
      
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToMotion.HandleMotionEvent.UnknownEvent",
                        "Reached unknown state %hhu.", event.GetData().GetTag());
    }
  }
}

} // namespace Cozmo
} // namespace Anki