/**
 * File: behaviorHighLevelAI.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Root behavior to handle the state machine for the high level AI of victor (similar to Cozmo's
 *              freeplay activities)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__

#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorHighLevelAI : public InternalStatesBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorHighLevelAI(const Json::Value& config);
  
public:

  virtual ~BehaviorHighLevelAI();
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }

private:
  
  PreDefinedStrategiesMap CreatePreDefinedStrategies();
  
};

}
}

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__
