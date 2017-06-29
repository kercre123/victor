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

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"

#include "clad/types/needsSystemTypes.h"
#include "json/json-forwards.h"

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

private:

  struct Params {
    Params(const Json::Value& config);
    
    // If this activity expresses a severe needs state, this will be set to non-Count and flagged in the
    // whiteboard when this activity starts or stops
    NeedId _severeNeedExpression = NeedId::Count;

    // whether or not to clear the expressed need when this activity exits.
    bool _clearSevereNeedExpressionOnExit = false;

    bool _shouldDisableReactions = false;
  };

  const Params _params;
};

}
}

#endif
