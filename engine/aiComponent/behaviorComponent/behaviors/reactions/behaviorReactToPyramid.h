/**
 * File: behaviorReactToPyramid.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-10-27
 *
 * Description: Cozmo reacts to seeing a pyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToPyramid_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToPyramid_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToPyramid : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToPyramid(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  float _nextValidReactionTime_s;
  
};

}// namespace Cozmo
}// namespace Anki

#endif
