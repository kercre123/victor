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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToStackOfCubes : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToStackOfCubes(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated() override;
  
private:
  TimeStamp_t _nextValidReactionTime_s;

};

}// namespace Cozmo
}// namespace Anki

#endif
