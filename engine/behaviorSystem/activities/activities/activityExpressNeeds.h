/**
 * File: activityExpressNeeds.h
 *
 * Author: Brad Neuman
 * Created: 2017-06-20
 *
 * Description: Activity to express severe needs states.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityExpressNeeds_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityExpressNeeds_H__

#include "engine/behaviorSystem/activities/activities/iActivity.h"

namespace Anki {
namespace Cozmo {

class ActivityExpressNeeds : public IActivity
{
public:
  ActivityExpressNeeds(Robot& robot, const Json::Value& config);
  virtual ~ActivityExpressNeeds() {};

protected:

  virtual void OnSelectedInternal(Robot& robot) override;
  virtual void OnDeselectedInternal(Robot& robot) override;

};

}
}

#endif
