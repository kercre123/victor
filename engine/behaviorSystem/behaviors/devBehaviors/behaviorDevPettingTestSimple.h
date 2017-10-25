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

#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "clad/types/touchGestureTypes.h"
#include <vector>

namespace Anki {
namespace Cozmo {
  
class IWantsToRunStrategy;
  
class BehaviorDevPettingTestSimple : public IBehavior
{
public:
  
  virtual ~BehaviorDevPettingTestSimple() { }
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
protected:
  
  friend class BehaviorContainer;
  BehaviorDevPettingTestSimple(Robot& robot, const Json::Value& config);
  
  virtual Result InitInternal(Robot& robot) override;

  virtual void StopInternal(Robot& robot) override;
  
  virtual BehaviorStatus UpdateInternal(Robot& robot) override;
  
private:
  
  // helper struct to organize the mapping between
  // touch-gesture and the animation metadata
  struct TouchGestureAnimationConfig
  {
    IWantsToRunStrategyPtr strategy;
    std::string animationName;
    float animationRate_s;
    float timeLastPlayed_s;
    
    TouchGestureAnimationConfig(IWantsToRunStrategy* sp,
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
  
  std::vector<TouchGestureAnimationConfig> _tgAnimConfigs;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__
