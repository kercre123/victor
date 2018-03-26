/**
 * File: behaviorAnimSequenceWithFace.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-05-23
 *
 * Description:  a sequence of animations after turning to a face (if possible)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimSequenceWithFace.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequenceWithFace::BehaviorAnimSequenceWithFace(const Json::Value& config)
: BaseClass(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequenceWithFace::OnBehaviorActivated()
{
  // attempt to turn towards last face, and even if fails, move on to the animations
  DelegateIfInControl(new TurnTowardsLastFacePoseAction(), [this]() {
      BaseClass::StartPlayingAnimations();
    });      
}

} // namespace Cozmo
} // namespace Anki
