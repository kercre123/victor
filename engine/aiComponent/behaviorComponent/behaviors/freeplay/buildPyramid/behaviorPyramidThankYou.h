/**
 * File: behaviorPyramidThankYou.h
 *
 * Author: Kevin M. Karol
 * Created: 01/24/16
 *
 * Description: Behavior to thank users for putting a block upright
 * when it was on its side for build pyramid
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPyramidThankYou_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPyramidThankYou_H__

#include "coretech/common/engine/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;

class BehaviorPyramidThankYou : public ICozmoBehavior
{
public:
  virtual ~BehaviorPyramidThankYou();
  void SetTargetID(s32 targetID){_targetID = targetID;}
  
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPyramidThankYou(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual bool WantsToBeActivatedBehavior() const override;
    
private:
  mutable s32 _targetID;
};


}
}


#endif
