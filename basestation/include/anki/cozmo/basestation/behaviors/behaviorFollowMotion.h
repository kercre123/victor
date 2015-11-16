/**
 * File: behaviorFollowMotion.h
 *
 * Author: Andrew Stein
 * Created: 11/13/15
 *
 * Description: Behavior for following motion in the image
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorFollowMotion_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFollowMotion_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorFollowMotion : public IBehavior
{
public:
  BehaviorFollowMotion(Robot& robot, const Json::Value& config);
  
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
  
protected:
    
  virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;
  
private:
  
  bool _interrupted = false;
  u8   _originalVisionModes = 0;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;

}; // class BehaviorFollowMotion
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFollowMotion_H__
