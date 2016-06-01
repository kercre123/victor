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

CONSOLE_VAR(f32, kBPDB_finalHeadAngle_deg, "Behavior.PutDownBlock.HeadAngle",  -20.0f);
CONSOLE_VAR(f32, kBPDB_backupDist_mm,      "Behavior.PutDownBlock.BackupDist", -30.0f);

static const char* const kLookAtFaceAnimGroup = "ag_lookAtFace_keepAlive";
static const float kScoreIncreaseDuringPutDown = 5.0f;
static const float kScoreIncreasePostPutdown = 0.6f;

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
              kScoreIncreaseDuringPutDown,
              &BehaviorPutDownBlock::LookDownAtBlock);
  return Result::RESULT_OK;
}


void BehaviorPutDownBlock::LookDownAtBlock(Robot& robot)
{
  StartActing(CreateLookAfterPlaceAction(robot), kScoreIncreasePostPutdown);
}

IActionRunner* BehaviorPutDownBlock::CreateLookAfterPlaceAction(Robot& robot)
{
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  if( robot.IsCarryingObject() ) {
    // glance down to see if we see the cube if we still think we are carrying
    static const int kNumFrames = 2;
    
    CompoundActionParallel* parallel = new CompoundActionParallel(robot,
                                                                  {new MoveHeadToAngleAction(robot, DEG_TO_RAD(kBPDB_finalHeadAngle_deg)),
                                                                   new DriveStraightAction(robot, kBPDB_backupDist_mm, DEFAULT_PATH_MOTION_PROFILE.speed_mmps)});
    action->AddAction(parallel);
    action->AddAction(new WaitForImagesAction(robot, kNumFrames));
  }

  // in any case, look back at the last face after this is done (to give them a chance to show another cube)
  const bool sayName = true;
  action->AddAction(new TurnTowardsFaceWrapperAction(robot,
                                                     new PlayAnimationGroupAction(robot, kLookAtFaceAnimGroup),
                                                     true, false, PI_F, sayName));
  return action;
}

void BehaviorPutDownBlock::StopInternal(Robot& robot)
{
}

}
}

