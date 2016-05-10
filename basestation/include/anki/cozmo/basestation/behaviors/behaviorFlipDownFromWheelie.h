/**
 * File: behaviorFlipDownFromWheelie.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-29
 *
 * Description: A behavior which can run when cozmo is up on his backpack (in wheelie mode) It will play an
 *              animation which flips him back down onto the ground
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFlipDownFromWheelie_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFlipDownFromWheelie_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorFlipDownFromWheelie : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorFlipDownFromWheelie(Robot& robot, const Json::Value& config);

public:

  virtual bool IsRunnableInternal(const Robot& robot) const override;

protected:

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  // only one state here so this is the "transition" function
  void Go(Robot& robot);
};

}
}


#endif
