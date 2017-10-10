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

#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {

class ActivityStrictPriority : public IActivity
{
public:
  ActivityStrictPriority(const Json::Value& config);
  virtual ~ActivityStrictPriority() {};

protected:
  // get next behavior by properly managing the sub-activities
  virtual ICozmoBehaviorPtr GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior) override;
  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnActivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;

private:
  using ActivityVector = std::vector< std::shared_ptr<IActivity>>;
  ActivityVector _activities;
  
  IActivity* _currentActivityPtr;
  ICozmoBehaviorPtr _behaviorWait;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityStrictPriority_H__
