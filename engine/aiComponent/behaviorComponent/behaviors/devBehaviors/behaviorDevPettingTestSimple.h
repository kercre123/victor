/**
 * File: BehaviorDevPettingTestSimple.h
 *
 * Author: Arjun Menon
 * Date:   09/12/2017
 *
 * Description: simple test behavior to respond to touch
 *              and petting input. Does nothing until a
 *              touch event comes in for it to react to
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/touchGestureTypes.h"
#include <vector>

namespace Anki {
namespace Cozmo {
  
class IStateConceptStrategy;
  
class BehaviorDevPettingTestSimple : public ICozmoBehavior
{
public:
  
  virtual ~BehaviorDevPettingTestSimple() { }
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
protected:
  
  friend class BehaviorContainer;
  BehaviorDevPettingTestSimple(const Json::Value& config);
  
  void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual BehaviorStatus UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  
  // helper struct to organize the mapping between
  // touch-gesture and the animation metadata
  struct TouchGestureAnimationConfig
  {
    IStateConceptStrategyPtr strategy;
    std::string animationName;
    float animationRate_s;
    float timeLastPlayed_s;
    
    TouchGestureAnimationConfig(IStateConceptStrategy* sp,
                                std::string animationName,
                                float animationRate_s,
                                float timeLastPlayed_s)
    : strategy(sp)
    , animationName(animationName)
    , animationRate_s(animationRate_s)
    , timeLastPlayed_s(timeLastPlayed_s)
    {
    }
    
  };
  
  Json::Value _configArray;
  std::vector<TouchGestureAnimationConfig> _tgAnimConfigs;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__
