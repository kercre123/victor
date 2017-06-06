/**
 * File: behaviorPlayAnimSequenceWithFace.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-05-23
 *
 * Description: Play a sequence of animations after turning to a face (if possible)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimSequenceWithFace.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"

namespace Anki {
namespace Cozmo {

BehaviorPlayAnimSequenceWithFace::BehaviorPlayAnimSequenceWithFace(Robot& robot, const Json::Value& config)
  : BaseClass(robot, config)
{
}

Result BehaviorPlayAnimSequenceWithFace::InitInternal(Robot& robot)
{
  // attempt to turn towards last face, and even if fails, move on to the animations
  StartActing(new TurnTowardsLastFacePoseAction(robot), [this](Robot& robot) {
      BaseClass::StartPlayingAnimations(robot);
    });

  return Result::RESULT_OK;      
}

}
}
