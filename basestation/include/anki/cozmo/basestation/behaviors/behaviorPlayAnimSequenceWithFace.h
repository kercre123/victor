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

#include "anki/cozmo/basestation/behaviors/behaviorPlayAnimSequence.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlayAnimSequenceWithFace : public BehaviorPlayAnimSequence
{
using BaseClass = BehaviorPlayAnimSequence;
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlayAnimSequenceWithFace(Robot& robot, const Json::Value& config);

public:

  virtual Result InitInternal(Robot& robot) override;
};

}
}

#endif
