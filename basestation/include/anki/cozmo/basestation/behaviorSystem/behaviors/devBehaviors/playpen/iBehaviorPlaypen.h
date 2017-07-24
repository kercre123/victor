/**
 * File: iBehaviorPlaypen.h
 *
 * Author: Al Chaussee
 * Created: 07/24/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__
#define __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class IBehaviorPlaypen : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  IBehaviorPlaypen(Robot& robot, const Json::Value& config);
  
//  virtual Result InitInternal(Robot& robot)   override;
  virtual Result ResumeInternal(Robot& robot) override { return RESULT_FAIL; }
//  virtual void   StopInternal(Robot& robot)   override;

  virtual bool CarryingObjectHandledInternally() override { return false; }
  
public:

  // TODO: Pass in results struct to populate
  virtual BehaviorStatus GetResults() = 0;
  
  
private:

  
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__
