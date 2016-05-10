/**
 * File: behaviorReactToStop.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-18
 *
 * Description: Reactionary behavior for the Stop event, which may be shortly followed by a cliff or pickup event
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToStop_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToStop_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToStop : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToStop(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  
protected:
    
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
}; // class BehaviorReactToStop
  

} // namespace Cozmo
} // namespace Anki

#endif
