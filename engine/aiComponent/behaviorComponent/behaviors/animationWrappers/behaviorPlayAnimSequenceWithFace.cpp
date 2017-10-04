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

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimSequenceWithFace.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"

namespace Anki {
namespace Cozmo {

BehaviorPlayAnimSequenceWithFace::BehaviorPlayAnimSequenceWithFace(const Json::Value& config)
: BaseClass(config)
{
}

Result BehaviorPlayAnimSequenceWithFace::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // attempt to turn towards last face, and even if fails, move on to the animations
  DelegateIfInControl(new TurnTowardsLastFacePoseAction(robot), [this](BehaviorExternalInterface& behaviorExternalInterface) {
      BaseClass::StartPlayingAnimations(behaviorExternalInterface);
    });

  return Result::RESULT_OK;      
}

}
}
