/**
 * File: behaviorPutDownBlock.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-05-23
 *
 * Description: Simple behavior which puts down a block (using an animation group)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorPutDownBlock.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(f32, kBPDB_finalHeadAngle_deg, "Behavior.PutDownBlock", -20.0f);

BehaviorPutDownBlock::BehaviorPutDownBlock(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
}

bool BehaviorPutDownBlock::IsRunnableInternal(const Robot& robot) const
{
  return robot.IsCarryingObject();
}

Result BehaviorPutDownBlock::InitInternal(Robot& robot)
{
  StartActing(new PlayAnimationGroupAction(robot, _putDownAnimGroup),
              &BehaviorPutDownBlock::LookDownAtBlock);
  return Result::RESULT_OK;
}


void BehaviorPutDownBlock::LookDownAtBlock(Robot& robot)
{
  StartActing(CreateLookAfterPlaceAction(robot));
}

IActionRunner* BehaviorPutDownBlock::CreateLookAfterPlaceAction(Robot& robot)
{
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  if( robot.IsCarryingObject() ) {
    // glance down to see if we see the cube if we still think we are carrying
    static const int kNumFrames = 2;
    
    action->AddAction(new MoveHeadToAngleAction(robot, DEG_TO_RAD(kBPDB_finalHeadAngle_deg)));
    action->AddAction(new WaitForImagesAction(robot, kNumFrames));
  }

  // in any case, look back at the last face after this is done (to give them a chance to show another cube)

  action->AddAction(new TurnTowardsLastFacePoseAction(robot));
  return action;
}

void BehaviorPutDownBlock::StopInternal(Robot& robot)
{
}

}
}

