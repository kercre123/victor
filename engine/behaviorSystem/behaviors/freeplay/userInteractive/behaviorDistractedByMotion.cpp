/**
 * File: BehaviorDistractedByMotion.cpp
 *
 * Author: Lorenzo Riano
 * Created: 9/20/17
 *
 * Description: This is a behavior that turns the robot towards motion. Motion can happen in
 *              different areas of the camera: top, left and right. The robot will either move
 *              in place towards the motion (for left and right) or will look up.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "behaviorDistractedByMotion.h"

#include "engine/actions/basicActions.h"
#include "engine/components/visionComponent.h"
#include "engine/robot.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

BehaviorDistractedByMotion::BehaviorDistractedByMotion(Anki::Cozmo::Robot &robot,
                                                       const Json::Value &config)
    : IBehavior(robot, config)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedMotion
  });
  PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion", "Constructor called");
}

void BehaviorDistractedByMotion::SetState_internal(State state, const std::string &stateName)
{
  _state = state;
  SetDebugStateName(stateName);
}

void BehaviorDistractedByMotion::HandleWhileRunning(const IBehavior::EngineToGameEvent &event, Robot &robot)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::RobotObservedMotion:
    {
      PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.HandleWhileNotRunning",
                    "New motion message");

      const ExternalInterface::RobotObservedMotion& motionObserved = event.GetData().Get_RobotObservedMotion();

      // TODO There's an implicit hierarchy here: top > right > left
      if (motionObserved.top_img_area > 0) {
        _observedX = motionObserved.top_img_x;
        _observedY = motionObserved.top_img_y;
        _motionObserved = true;
        PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.HandleWhileNotRunning",
                      "Motion observed in top image coordinates: (%d, %d)", _observedX, _observedY);

        if (_state != State::TurningTowardsMotion) {
          StopActing();
        }
      }
      else if (motionObserved.right_img_area > 0) {
        _observedX = motionObserved.right_img_x;
        _observedY = motionObserved.right_img_y;
        _motionObserved = true;
        PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.HandleWhileNotRunning",
                      "Motion observed in right image coordinates: (%d, %d)", _observedX, _observedY);
        if (_state != State::TurningTowardsMotion) {
          StopActing();
        }
      }
      else if (motionObserved.left_img_area > 0) {
        _observedX = motionObserved.left_img_x;
        _observedY = motionObserved.left_img_y;
        _motionObserved = true;
        PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.HandleWhileNotRunning",
                      "Motion observed inleft  image coordinates: (%d, %d)", _observedX, _observedY);
        if (_state != State::TurningTowardsMotion) {
          StopActing();
        }
      }
      else {
        _motionObserved = false;
      }
      break;
    }
    default: {
      PRINT_NAMED_ERROR("BehaviorDistractedByMotion.HandleWhileNotRunning.InvalidEvent", "");
      break;
    }
  }
}

bool BehaviorDistractedByMotion::CarryingObjectHandledInternally() const
{
  return false;
}

Result BehaviorDistractedByMotion::InitInternal(Robot &robot)
{
  PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.InitInternal", "InitInternal called");

  TransitionToStartState(robot);
  return Result::RESULT_OK;
}

bool BehaviorDistractedByMotion::IsRunnableInternal(const Robot &robot) const
{
  return true;
}

void BehaviorDistractedByMotion::TransitionToStartState(Robot &robot)
{
  SET_STATE(Starting);
  PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.TransitionToStartState",
                "Perform action");
  if (_motionObserved) {
    TransitionToTurnsTowardsMotion(robot);
  }
  else {
    TransitionToWaitForMotion(robot);
  }

}

void BehaviorDistractedByMotion::TransitionToEnd(Robot &robot)
{
  SET_STATE(Ending);
  PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.TranitionToEnd",
                "End of DistractedByMotion behavior");
}

void BehaviorDistractedByMotion::TransitionToWaitForMotion(Robot &robot)
{
  SET_STATE(WaitingForMotion);
  static const float kWaitForMotionInterval_s = 10.0f;
  StartActing(new WaitAction(robot, kWaitForMotionInterval_s),
              &BehaviorDistractedByMotion::TransitionToAfterWaitingForMotion);
}

void BehaviorDistractedByMotion::TransitionToAfterWaitingForMotion(Robot &robot)
{
  SET_STATE(AfterWaitingForMotion);
  if (_motionObserved) {
    PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.TransitionToAfterWaitingForMotion",
                  "Motion has happened!");
    TransitionToTurnsTowardsMotion(robot);
  }
  else {
    PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.TransitionToAfterWaitingForMotion",
                  "No motion observed, bailing out");
    TransitionToEnd(robot);
  }
}

void BehaviorDistractedByMotion::TransitionToTurnsTowardsMotion(Robot &robot)
{
  SET_STATE(TurningTowardsMotion);

  const Point2f motionCentroid(_observedX, _observedY);
  Radians relPanAngle;
  Radians relTiltAngle;

  // TODO This is bad, and needs to be solved (see VIC-323)
  {
    const Vision::Camera &camera = robot.GetVisionComponent().GetCamera();
    const Point2f cameraCentroid = camera.GetCalibration()->GetCenter();
    const Point2f centeredMotion = motionCentroid - cameraCentroid;
    camera.ComputePanAndTiltAngles(centeredMotion, relPanAngle, relTiltAngle);
  }

  PRINT_CH_INFO("Behaviors", "BehaviorDistractedByMotion.TransitionToTurnsTowardsMotion",
                "Turning towards %f pan and %f tilt", relPanAngle.getDegrees(), relTiltAngle.getDegrees());

  IActionRunner* action = new PanAndTiltAction(robot, relPanAngle, relTiltAngle, false, false);

  // clear motion
  _motionObserved = false;

  StartActing(action, &BehaviorDistractedByMotion::TransitionToWaitForMotion);

}

} //namespace Anki
} //namespace Cozmo
