/**
 * File: ActivityBehaviorsOnly.h
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Activity when choosing the next behavior is all that's desired
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityBehaviorsOnly_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityBehaviorsOnly_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {

class ActivityBehaviorsOnly : public IActivity
{
public:
  ActivityBehaviorsOnly(Robot& robot, const Json::Value& config);
  ~ActivityBehaviorsOnly() {};
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityBehaviorsOnly_H__
