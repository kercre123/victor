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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToPyramid : public IBehavior
{
private:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToPyramid(const Json::Value& config);

public:
  virtual bool IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return false;}
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  float _nextValidReactionTime_s;
  
};

}// namespace Cozmo
}// namespace Anki

#endif
