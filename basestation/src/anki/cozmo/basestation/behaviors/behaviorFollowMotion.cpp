/**
 * File: behaviorFollowMotion.cpp
 *
 * Author: Andrew Stein
 * Created: 11/13/15
 *
 * Description: Behavior for following motion in the image
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorFollowMotion.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static std::vector<std::string> _animReactions = {
  "Demo_Motion_Reaction",
};

BehaviorFollowMotion::BehaviorFollowMotion(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  _name = "FollowMotion";
  
  SubscribeToTags({
    MessageEngineToGameTag::RobotObservedMotion,
    MessageEngineToGameTag::RobotCompletedAction
  });
}

bool BehaviorFollowMotion::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  return true;
}

Result BehaviorFollowMotion::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
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

IBehavior::Status BehaviorFollowMotion::UpdateInternal(Robot& robot, double currentTime_sec)
{
  if(_interrupted) {
    // Restore original vision modes
    robot.GetVisionComponent().SetModes(_originalVisionModes);
    return Status::Complete;
  } else {
    return Status::Running;
  }
}

Result BehaviorFollowMotion::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  _interrupted = true;
  return Result::RESULT_OK;
}

void BehaviorFollowMotion::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion:
    {
      // Ignore motion while an action is running
      if(_actionRunning == 0) {
        const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
        
        const Point2f motionCentroid(motionObserved.img_x, motionObserved.img_y);
        
        // Robot gets more happy/excited and less calm when he sees motion.
        // (May not want to do this every time he sees motion, since it'll increase
        //  pretty fast)
        robot.GetMoodManager().AddToEmotions(EmotionType::Happy,   kEmotionChangeSmall,
                                             EmotionType::Excited, kEmotionChangeSmall,
                                             EmotionType::Calm,   -kEmotionChangeSmall, "MotionReact");
        
        // Turn to face the motion:
        // Convert image positions to desired relative angles
        const Vision::CameraCalibration& calibration = robot.GetVisionComponent().GetCameraCalibration();
        const Radians relHeadAngle_rad = std::atan(-motionCentroid.y() / calibration.GetFocalLength_y());
        const Radians relBodyPanAngle_rad = std::atan(-motionCentroid.x() / calibration.GetFocalLength_x());
        
        
        IAction* action = nullptr;
        
        if(relHeadAngle_rad.getAbsoluteVal() < _driveForwardTol &&
           relBodyPanAngle_rad.getAbsoluteVal() < _driveForwardTol)
        {
          // Move towards the motion since it's centered
          Pose3d newPose(robot.GetPose());
          const f32 angleRad = newPose.GetRotation().GetAngleAroundZaxis().ToFloat();
          newPose.SetTranslation({newPose.GetTranslation().x() + _moveForwardDist_mm*std::cos(angleRad),
            newPose.GetTranslation().y() + _moveForwardDist_mm*std::sin(angleRad),
            newPose.GetTranslation().z()});
          PathMotionProfile motionProfile(DEFAULT_PATH_MOTION_PROFILE);
          motionProfile.speed_mmps *= _moveForwardSpeedIncrease; // Drive forward a little faster than normal
          
          action = new DriveToPoseAction(newPose, motionProfile,
                                         false, false, 50, DEG_TO_RAD(45));
          
        } else {
          PanAndTiltAction* panTiltAction = new PanAndTiltAction(relBodyPanAngle_rad, relHeadAngle_rad,
                                                                 false, false);
          panTiltAction->SetPanTolerance(_panAndTiltTol);
          panTiltAction->SetTiltTolerance(_panAndTiltTol);
          action = panTiltAction;
        }
        
        ASSERT_NAMED(nullptr != action, "Action pointer should not be null at this point");
        
        _actionRunning = action->GetTag();
        
        robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, action);
      }
      break;
    }
      
    case MessageEngineToGameTag::RobotCompletedAction:
    {
      // If the action we were running completes, allow us to respond to motion again
      if(event.GetData().Get_RobotCompletedAction().idTag == _actionRunning) {
        _actionRunning = 0;
      }
      break;
    }
      
    default:
    {
      PRINT_NAMED_ERROR("BehaviorFollowMotion.HandleMotionEvent.UnknownEvent",
                        "Reached unknown state %hhu.", event.GetData().GetTag());
    }
  }
}

} // namespace Cozmo
} // namespace Anki