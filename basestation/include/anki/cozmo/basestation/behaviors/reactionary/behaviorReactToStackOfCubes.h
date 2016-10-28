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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToStackOfCubes : public IReactionaryBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToStackOfCubes(Robot& robot, const Json::Value& config);

public:
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool ShouldRunWhileOffTreads() const override { return false;}
  
protected:
  virtual Result InitInternalReactionary(Robot& robot) override;
  
private:
  TimeStamp_t _nextValidReactionTime_s;

};

}// namespace Cozmo
}// namespace Anki

#endif
