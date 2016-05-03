/**
 * File: behaviorFlipDownFromWheelie.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-29
 *
 * Description: A behavior which can run when cozmo is up on his backpack (in wheelie mode) It will play an
 *              animation which flips him back down onto the ground
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorFlipDownFromWheelie.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

static const float kPitchAngleToConsiderOnBack_rads = DEG_TO_RAD(60.0f);
static const float kMinLiftHeightForFlip_mm = 72.0f;
static const float kLiftHeightUpDuration_s = 1.2f;

BehaviorFlipDownFromWheelie::BehaviorFlipDownFromWheelie(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("FlipDownFromWheelie");
}

bool BehaviorFlipDownFromWheelie::IsRunnable(const Robot& robot) const
{
  return robot.GetPitchAngle() > kPitchAngleToConsiderOnBack_rads;
}

Result BehaviorFlipDownFromWheelie::InitInternal(Robot& robot)
{
  Go(robot);
  return Result::RESULT_OK;
}

void BehaviorFlipDownFromWheelie::StopInternal(Robot& robot)
{
}


void BehaviorFlipDownFromWheelie::Go(Robot& robot)
{

  // first check if we are still on our back
  if( !IsRunnable(robot) ) {
    // we are finished! Make sure the lift is lowered before we exit
    MoveLiftToHeightAction* action = new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK);
    // TODO:(bn) just use the same slow movement for now, this will all be replaced anyway
    // let the behavior exit after this action
    action->SetDuration(kLiftHeightUpDuration_s);
    StartActing(action);
    return;
  }
  
  // TODO:(bn) use an animation here

  // if lift is low, raise the lift
  if( robot.GetLiftHeight() < kMinLiftHeightForFlip_mm ) {
    MoveLiftToHeightAction* action = new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::HIGH_DOCK);
    action->SetDuration(kLiftHeightUpDuration_s);
    StartActing(action, &BehaviorFlipDownFromWheelie::Go);
  }
  else {
    StartActing(new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK),
                &BehaviorFlipDownFromWheelie::Go);
  }
}

}
}

