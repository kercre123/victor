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
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

static const float kInitialDriveSpeed  = 80.0f;

#define SET_STATE(s) SetState_internal(State::s, #s)

BehaviorDemoFearEdge::BehaviorDemoFearEdge(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("DemoFearEdge");
}

Result BehaviorDemoFearEdge::ResumeInternal(Robot& robot)
{
  // if we are resuming, this probably means we saw a cliff, so we don't want to do anything. Return OK, but
  // don't start an action so the behavior will complete immediately
  return Result::RESULT_OK;
}

Result BehaviorDemoFearEdge::InitInternal(Robot& robot)
{
  TransitionToDrivingForward(robot);
  return Result::RESULT_OK;
}

void BehaviorDemoFearEdge::StopInternal(Robot& robot)
{
}


void BehaviorDemoFearEdge::TransitionToDrivingForward(Robot& robot)
{
  SET_STATE(DrivingForward);

  // drive pretty far (hopefully hitting the cliff), and just repeat if we don't hit it. We expect to be
  // interrupted by the cliff behavior

  // TODO:(bn) a better path here
  StartActing(new DriveStraightAction(robot, 1000.0f, kInitialDriveSpeed),
              &BehaviorDemoFearEdge::TransitionToDrivingForward);
}

void BehaviorDemoFearEdge::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorDemoFearEdge.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}

}
}

