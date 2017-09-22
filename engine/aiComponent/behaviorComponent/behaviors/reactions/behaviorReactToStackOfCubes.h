/**
 * File: behaviorReactToStackOfCubes.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-10-27
 *
 * Description: Cozmo reacts to seeing a stack of cubes
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToStackOfCubes_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToStackOfCubes_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToStackOfCubes : public IBehavior
{
private:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToStackOfCubes(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return false;}
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  TimeStamp_t _nextValidReactionTime_s;

};

}// namespace Cozmo
}// namespace Anki

#endif
