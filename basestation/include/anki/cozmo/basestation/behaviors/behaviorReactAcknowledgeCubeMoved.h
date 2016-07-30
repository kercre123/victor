/**
 * File: behaviorReactAcknowledgeCubeMoved.h
 *
 * Author: Kevin M. Karol
 * Created: 7/26/16
 *
 * Description: Behavior to acknowledge when a localized cube has been moved
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactAcknowledgeCubeMoved : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactAcknowledgeCubeMoved(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  
protected:
  
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;
  
  virtual void AlwaysHandleInternal(const RobotToEngineEvent& event, const Robot& robot) override;
  
private:
  
}; // class BehaviorReactAcknowledgeCubeMoved

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__