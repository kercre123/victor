/**
 * File: BehaviorReactToImpact.h
 *
 * Author: Matt Michini
 * Date:   10/9/2017
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToImpact_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToImpact_H__

#include "engine/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToImpact : public IBehavior
{
public:
  
  virtual ~BehaviorReactToImpact() { }
  virtual bool IsRunnableInternal(const Robot& robot ) const override { return true; }
  virtual bool CarryingObjectHandledInternally() const override { return false;}

protected:
  
  friend class BehaviorContainer;
  BehaviorReactToImpact(Robot& robot, const Json::Value& config);
  
  virtual Result InitInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;
  virtual void AlwaysHandle(const EngineToGameEvent&, const Robot&) override;

private:
  
  void TransitionToPlayingAnim(Robot& robot);
  void ResetFlags();
  
  bool _doneMotorCalibration = false;
  bool _doneFalling = false;
  bool _playImpactAnim = false;

};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToImpact_H__
