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

namespace Anki {
namespace Cozmo {

BehaviorDistractedByMotion::BehaviorDistractedByMotion(Anki::Cozmo::Robot &robot,
                                                       const Json::Value &config)
    : IBehavior(robot, config)
{
}

bool BehaviorDistractedByMotion::CarryingObjectHandledInternally() const
{
  return false;
}

Result BehaviorDistractedByMotion::InitInternal(Robot &robot)
{

  // TODO add the first transition here

  return Result::RESULT_OK;
}

bool BehaviorDistractedByMotion::IsRunnableInternal(const Robot &robot) const
{
  return true;
}


} //namespace Anki
} //namespace Cozmo
