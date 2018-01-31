/**
 * File: behaviorPlayAnimSequenceWithObject.h
 *
 * Author: Matt Michini
 * Created: 2018-01-11
 *
 * Description: Play a sequence of animations after turning to an object (if possible)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Behaviors_BehaviorPlayAnimSequenceWithObject_H__
#define __Engine_Behaviors_BehaviorPlayAnimSequenceWithObject_H__

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimSequence.h"

#include "clad/types/objectTypes.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlayAnimSequenceWithObject : public BehaviorPlayAnimSequence
{
using BaseClass = BehaviorPlayAnimSequence;
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlayAnimSequenceWithObject(const Json::Value& config);
  
public:
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  
  const ObservableObject* GetLocatedObject() const;
  
private:
  
  // The ObjectType to look for
  ObjectType _objectType = ObjectType::UnknownObject;
  
};


}
}

#endif // __Engine_Behaviors_BehaviorPlayAnimSequenceWithObject_H__
