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
  SetDefaultName("FollowMotion");

  SubscribeToTags({{
    EngineToGameTag::RobotObservedMotion,
    EngineToGameTag::RobotCompletedAction
  }});
}
  
bool BehaviorFollowMotion::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  return true;
}

float BehaviorFollowMotion::EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const
{
  return 0.3f; // for the investor demo, just use a fixed score
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
  
  // Do the initial reaction for first motion each time we restart this behavior
  // (but only if it's been long enough since last interruption)
  if(currentTime_sec - _lastInterruptTime_sec > _initialReactionWaitTime_sec) {
    _initialReactionAnimPlayed = false;
  } else {
    _initialReactionAnimPlayed = true;
  }

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
  _actionRunning = 0;
  _interrupted = true;
  _lastInterruptTime_sec = currentTime_sec;
  
  return Result::RESULT_OK;
}

void BehaviorFollowMotion::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion:
    {
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      
      // Ignore motion while an action is running
      if(_actionRunning == 0 && motionObserved.img_area > 0)
      {
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
        Radians relHeadAngle_rad    = std::atan(-motionCentroid.y() / calibration.GetFocalLength_y());
        Radians relBodyPanAngle_rad = std::atan(-motionCentroid.x() / calibration.GetFocalLength_x());
        
        /* I don't believe this is necessary because we only see motion while stopped
        // Those angles are relative to where the robot was when he saw the motion.
        // Figure out what those are relative to _now_
        RobotPoseStamp *p;
        if(RESULT_OK != robot.GetPoseHistory()->GetComputedPoseAt(motionObserved.timestamp, &p)) {
          PRINT_NAMED_WARNING("BehaviorFollowMotion.HandeWhileRunning.PoseHistoryError",
                              "Could not get historical pose for observed motion at t=%d",
                              motionObserved.timestamp);
        } else {
          relHeadAngle_rad += p->GetHeadAngle() - robot.GetHeadAngle();
          relBodyPanAngle_rad += p->GetPose().GetRotation().GetAngleAroundZaxis() - robot.GetPose().GetRotation().GetAngleAroundZaxis();
        }
         */
        
        PRINT_NAMED_INFO("BehaviorFollowMotion.HandleWhileRunning.Motion",
                         "Motion area=%d, centroid=(%.1f,%.1f), HeadTilt=%.1fdeg, BodyPan=%.1fdeg",
                         motionObserved.img_area,
                         motionCentroid.x(), motionCentroid.y(),
                         relHeadAngle_rad.getDegrees(), relBodyPanAngle_rad.getDegrees());
        
        IAction* action = nullptr;
        
        if(relHeadAngle_rad.getAbsoluteVal() < _driveForwardTol &&
           relBodyPanAngle_rad.getAbsoluteVal() < _driveForwardTol)
        {
          // Move towards the motion since it's centered
          DriveStraightAction* driveAction = new DriveStraightAction(_moveForwardDist_mm, DEFAULT_PATH_SPEED_MMPS*_moveForwardSpeedIncrease);
          driveAction->SetAccel(DEFAULT_PATH_ACCEL_MMPS2*_moveForwardSpeedIncrease);
          
          action = driveAction;
          
        } else {
          PanAndTiltAction* panTiltAction = new PanAndTiltAction(relBodyPanAngle_rad, relHeadAngle_rad,
                                                                 false, false);
          panTiltAction->SetPanTolerance(_panAndTiltTol);
          panTiltAction->SetTiltTolerance(_panAndTiltTol);
          action = panTiltAction;
        }
        
        ASSERT_NAMED(nullptr != action, "Action pointer should not be null at this point");
        
        robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, action);
        
        if(!_initialReactionAnimPlayed) {
          const Radians finalHeadAngle = robot.GetHeadAngle() + relHeadAngle_rad;
          
          PRINT_NAMED_INFO("BehaviorFollowMotion.HandleWhileRunning.FirstMotion",
                           "Queuing first motion reaction animation and head tilt back to %.1fdeg",
                           finalHeadAngle.getDegrees());
          
          // If this is the first motion reaction, also play the first part of the
          // motion reaction animation, move the head back to the right tilt, and
          // then play the second half of the animation to open the eyes back up
          CompoundActionSequential* compoundAction = new CompoundActionSequential({
            new PlayAnimationAction("ID_MotionFollow_ReactToMotion"),
            new MoveHeadToAngleAction(finalHeadAngle, _panAndTiltTol),
            new PlayAnimationAction("ID_MotionFollow_ReactToMotion_end")
          });
          
          // Wait for the last action to finish in this case
          _actionRunning = compoundAction->GetTag();
          
          robot.GetActionList().QueueActionNext(Robot::DriveAndManipulateSlot, compoundAction);
        } else {
          _actionRunning = action->GetTag();
        }
        
        ASSERT_NAMED(_actionRunning != 0, "Expecting action tag to be non-zero!");
      }
      break;
    }
      
    case MessageEngineToGameTag::RobotCompletedAction:
    {
      auto const& completedAction = event.GetData().Get_RobotCompletedAction();
      
      // If the action we were running completes, allow us to respond to motion again
      if(completedAction.idTag == _actionRunning) {
        _actionRunning = 0;
        
        if(completedAction.actionType == RobotActionType::COMPOUND) {
          _initialReactionAnimPlayed = true;
        }
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
