/**
 * File: behaviorAnimSequenceWithObject.h
 *
 * Author: Matt Michini
 * Created: 2018-01-11
 *
 * Description:  a sequence of animations after turning to an object (if possible)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Behaviors_BehaviorAnimSequenceWithObject_H__
#define __Engine_Behaviors_BehaviorAnimSequenceWithObject_H__

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimSequence.h"

#include "clad/types/objectTypes.h"

namespace Anki {
namespace Cozmo {

class BehaviorAnimSequenceWithObject : public BehaviorAnimSequence
{
using BaseClass = BehaviorAnimSequence;
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAnimSequenceWithObject(const Json::Value& config);
  virtual void OnBehaviorActivated() override;
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

public:
  virtual bool WantsToBeActivatedBehavior() const override;
  const ObservableObject* GetLocatedObject() const;
  
private:
  struct InstanceConfig {
    InstanceConfig();
    // The ObjectType to look for
    ObjectType objectType;
  };

  struct DynamicVariables {
    DynamicVariables();
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorAnimSequenceWithObject_H__
