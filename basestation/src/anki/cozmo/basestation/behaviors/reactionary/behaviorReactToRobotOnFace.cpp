/**
 * File: behaviorReactToRobotOnFace.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: Allows Cozmo to right himself when placed on his face
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToRobotOnFace.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

//Constants for on face - Note these values are duplicated in robot.cpp
static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;
static const float kPitchAngleOnFaceTurtleMin_rads = DEG_TO_RAD(90.f);
static const float kPitchAngleOnFaceTurtleMax_rads = DEG_TO_RAD(-110.f);
static const float kPitchAngleOnFaceTurtleMin_sim_rads = DEG_TO_RAD(90.f); //This has not been tested
static const float kPitchAngleOnFaceTurtleMax_sim_rads = DEG_TO_RAD(-110.f); //This has not been tested
  
BehaviorReactToRobotOnFace::BehaviorReactToRobotOnFace(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToRobotOnFace");

  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::RobotOnFace
  });
}

bool BehaviorReactToRobotOnFace::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToRobotOnFace::InitInternalReactionary(Robot& robot)
{
  FlipOverIfNeeded(robot);
  return Result::RESULT_OK;
}

void BehaviorReactToRobotOnFace::FlipOverIfNeeded(Robot& robot)
{
  if( robot.IsOnFace() ) {
    AnimationTrigger anim;
    const float turtleRollMinAngle = robot.IsPhysical() ? kPitchAngleOnFaceTurtleMin_rads : kPitchAngleOnFaceTurtleMin_sim_rads;
    const float turtleRollMaxAngle = robot.IsPhysical() ? kPitchAngleOnFaceTurtleMax_rads : kPitchAngleOnFaceTurtleMax_sim_rads;
    if(robot.GetPitchAngle() > turtleRollMinAngle || robot.GetPitchAngle() < turtleRollMaxAngle){
      anim = AnimationTrigger::FacePlantRoll;
    }else{
      anim = AnimationTrigger::FacePlantRollArmUp;
    }
    
    StartActing(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnFace::DelayThenCheckState);
  }
}

void BehaviorReactToRobotOnFace::DelayThenCheckState(Robot& robot)
{
  if( robot.IsOnFace() ) {
    StartActing(new WaitAction(robot, kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnFace::CheckFlipSuccess);
  }

}
  
void BehaviorReactToRobotOnFace::CheckFlipSuccess(Robot& robot)
{
  if( robot.IsOnFace() ) {
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FailedToRightFromFace),
                &BehaviorReactToRobotOnFace::FlipOverIfNeeded);
  }else{
    BehaviorObjectiveAchieved();
  }
}

bool BehaviorReactToRobotOnFace::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  if( event.GetTag() != MessageEngineToGameTag::RobotOnFace ) {
    PRINT_NAMED_ERROR("BehaviorReactToRobotOnFace.ShouldRunForEvent.BadEventType",
                      "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
    return false;
  }

  return true;
}

void BehaviorReactToRobotOnFace::StopInternalReactionary(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
