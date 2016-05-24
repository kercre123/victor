/**
 * File: behaviorPutDownBlock.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-23
 *
 * Description: Simple behavior which puts down a block (using an animation group)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPutDownBlock_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPutDownBlock_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorPutDownBlock : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPutDownBlock(Robot& robot, const Json::Value& config);

  virtual bool IsRunnableInternal(const Robot& robot) const override;

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

private:

  void LookDownAtBlock(Robot& robot);
  
  std::string _putDownAnimGroup = "ag_putDownBlock";

};

}
}

#endif
