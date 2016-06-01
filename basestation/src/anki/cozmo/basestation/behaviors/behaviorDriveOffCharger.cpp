/**
 * File: behaviorDriveOffCharger.cpp
 *
 * Author: Molly Jameson
 * Created: 2016-05-19
 *
 * Description: Behavior to drive to the edge off a charger and deal with the firmware cliff stop
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/behaviors/behaviorDriveOffCharger.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

static const float kInitialDriveSpeed = 100.0f;
static const float kInitialDriveAccel = 40.0f;

#define SET_STATE(s) SetState_internal(State::s, #s)

BehaviorDriveOffCharger::BehaviorDriveOffCharger(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("DriveOffCharger");
}
  
bool BehaviorDriveOffCharger::IsRunnableInternal(const Robot& robot) const
{
  return robot.IsOnChargerPlatform();
}

// Only want to run first if we're on the charger contacts, once running use the flat score in base EvaluateRunningScore
float BehaviorDriveOffCharger::EvaluateScoreInternal(const Robot& robot) const
{
  if( robot.IsOnCharger())
  {
    return IBehavior::EvaluateScoreInternal(robot);
  }
  return 0.f;
}

Result BehaviorDriveOffCharger::InitInternal(Robot& robot)
{
  TransitionToDrivingForward(robot);
  robot.GetBehaviorManager().GetWhiteboard().DisableCliffReaction(this);
  return Result::RESULT_OK;
}
  
void BehaviorDriveOffCharger::StopInternal(Robot& robot)
{
  robot.GetDrivingAnimationHandler().PopDrivingAnimations();
  robot.GetBehaviorManager().GetWhiteboard().RequestEnableCliffReaction(this);
}


Result BehaviorDriveOffCharger::ResumeInternal(Robot& robot)
{
  // We hit the end of the charger, just keep driving.
  TransitionToDrivingForward(robot);
  return Result::RESULT_OK;
}


  
IBehavior::Status BehaviorDriveOffCharger::UpdateInternal(Robot& robot)
{
  if( robot.IsOnChargerPlatform() )
  {
    return Status::Running;
  }
  return Status::Complete;
}


void BehaviorDriveOffCharger::TransitionToDrivingForward(Robot& robot)
{
  SET_STATE(DrivingForward);
  if( robot.IsOnChargerPlatform() )
  {
    robot.GetDrivingAnimationHandler().PushDrivingAnimations({_startDrivingAnimGroup,
                                                              _drivingLoopAnimGroup,
                                                              _stopDrivingAnimGroup});
    // probably interrupted by getting off the charger platform
    DriveStraightAction* action = new DriveStraightAction(robot, 100.0f, kInitialDriveSpeed);
    action->SetAccel(kInitialDriveAccel);
    StartActing(action, &BehaviorDriveOffCharger::TransitionToDrivingForward);
  }
}


void BehaviorDriveOffCharger::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorDriveOffCharger.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}

}
}

