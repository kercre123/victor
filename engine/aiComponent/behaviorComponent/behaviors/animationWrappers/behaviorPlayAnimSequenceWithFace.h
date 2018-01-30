/**
 * File: behaviorPlayAnimSequenceWithFace.h
 *
 * Author: Brad Neuman
 * Created: 2017-05-23
 *
 * Description: Play a sequence of animations after turning to a face (if possible)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlayAnimSequenceWithFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlayAnimSequenceWithFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimSequence.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlayAnimSequenceWithFace : public BehaviorPlayAnimSequence
{
using BaseClass = BehaviorPlayAnimSequence;
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlayAnimSequenceWithFace(const Json::Value& config);

public:

  virtual void OnBehaviorActivated() override;
  void SetAnimationsToPlay(const std::vector<AnimationTrigger>& animations){
    _animTriggers = animations;
  }
};

}
}

#endif
