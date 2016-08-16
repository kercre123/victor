/**
 * File: behaviorDemoFearEdge.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-27
 *
 * Description: Behavior to drive to the edge of the table and react (for the announce demo)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/behaviors/behaviorDemoFearEdge.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

static const float kInitialDriveSpeed = 100.0f;
static const float kInitialDriveAccel = 40.0f;

static const float kBackupDriveDist_mm = 200.0f;
static const float kBackupDriveSpeed_mmps = 100.0f;
static const float kBackupDriveAccel = 500.0f;
static const float kBackupDriveDecel = 100.0f;

#define SET_STATE(s) SetState_internal(State::s, #s)

BehaviorDemoFearEdge::BehaviorDemoFearEdge(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("DemoFearEdge");
}

Result BehaviorDemoFearEdge::ResumeInternal(Robot& robot)
{
  // If we are resuming that means we hit a cliff, so drive back
  TransitionToBackingUpForPounce(robot);
  return Result::RESULT_OK;
}

Result BehaviorDemoFearEdge::InitInternal(Robot& robot)
{
  TransitionToDrivingForward(robot);
  return Result::RESULT_OK;
}

void BehaviorDemoFearEdge::StopInternal(Robot& robot)
{
  robot.GetDrivingAnimationHandler().PopDrivingAnimations();
}

void BehaviorDemoFearEdge::TransitionToDrivingForward(Robot& robot)
{
  SET_STATE(DrivingForward);

  // drive pretty far (hopefully hitting the cliff), and just repeat if we don't hit it. We expect to be
  // interrupted by the cliff behavior

  // TODO:(bn) a better path here
  robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::DriveStartLaunch,
                                                            AnimationTrigger::DriveLoopLaunch,
                                                            AnimationTrigger::DriveEndLaunch});

  DriveStraightAction* action = new DriveStraightAction(robot, 1000.0f, kInitialDriveSpeed);
  action->SetAccel(kInitialDriveAccel);
  StartActing(action, &BehaviorDemoFearEdge::TransitionToDrivingForward);
}

void BehaviorDemoFearEdge::TransitionToBackingUpForPounce(Robot& robot)
{
  SET_STATE(BackingUpForPounce);

  DriveStraightAction* action = new DriveStraightAction(robot, -kBackupDriveDist_mm, kBackupDriveSpeed_mmps);
  action->SetAccel(kBackupDriveAccel);
  action->SetDecel(kBackupDriveDecel);
  StartActing(action, &BehaviorDemoFearEdge::TransitionToFinished);
}

void BehaviorDemoFearEdge::TransitionToFinished()
{
  SET_STATE(Finished);
}

void BehaviorDemoFearEdge::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorDemoFearEdge.TransitionTo", "%s", stateName.c_str());
  SetDebugStateName(stateName);
}

}
}

