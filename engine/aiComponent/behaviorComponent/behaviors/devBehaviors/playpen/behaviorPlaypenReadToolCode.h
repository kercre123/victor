/**
 * File: behaviorPlaypenReadToolCode.h
 *
 * Author: Al Chaussee
 * Created: 08/14/17
 *
 * Description: Checks that Cozmo can read the lift tool codes and that they are within expected bounds
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenReadToolCode_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenReadToolCode_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenReadToolCode : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenReadToolCode(const Json::Value& config);
  
  virtual Result OnBehaviorActivatedInternal()   override;
  
private:
  
  void TransitionToToolCodeRead(const ExternalInterface::RobotCompletedAction& rca);
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenReadToolCode_H__
