/**
* File: ActivityStrictPriority.h
*
* Author: Kevin M. Karol
* Created: 08/22/17
*
* Description: Activity for selecting sub-activities using a strict priority
* runnability metric
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityStrictPriority_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityStrictPriority_H__

#include "engine/aiComponent/behaviorSystem/activities/activities/iActivity.h"


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {

class ActivityStrictPriority : public IActivity
{
public:
  ActivityStrictPriority(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  virtual ~ActivityStrictPriority() {};

protected:
  // get next behavior by properly managing the sub-activities
  virtual IBehaviorPtr GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior) override;
  
private:
  using ActivityVector = std::vector< std::unique_ptr<IActivity>>;
  ActivityVector _activities;
  
  IActivity* _currentActivityPtr;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityStrictPriority_H__
