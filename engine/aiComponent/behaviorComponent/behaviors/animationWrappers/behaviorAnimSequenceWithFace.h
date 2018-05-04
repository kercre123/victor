/**
 * File: behaviorAnimSequenceWithFace.h
 *
 * Author: Brad Neuman
 * Created: 2017-05-23
 *
 * Description: Play a sequence of animations after turning to a face (if possible)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAnimSequenceWithFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAnimSequenceWithFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimSequence.h"

namespace Anki {
namespace Cozmo {

class BehaviorAnimSequenceWithFace : public BehaviorAnimSequence
{
  using BaseClass = BehaviorAnimSequence;
  
public:
  ~BehaviorAnimSequenceWithFace();
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAnimSequenceWithFace(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;

  virtual void OnBehaviorActivated() override;

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

private:

  struct InstanceConfig;
  std::unique_ptr<InstanceConfig> _iConfig;
  
};

}
}

#endif
