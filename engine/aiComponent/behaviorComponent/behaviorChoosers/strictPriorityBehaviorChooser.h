/**
* File: StrictPriorityBehaviorChooser.h
*
* Author: Kevin M. Karol
* Created: 05/18/2017
*
* Description: Class for handling picking of behaviors strictly based on their priority
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_StrictPriorityBehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_StrictPriorityBehaviorChooser_H__

#include "engine/aiComponent/behaviorComponent/behaviorChoosers/iBehaviorChooser.h"


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {
  
class ICozmoBehavior;
  
class StrictPriorityBehaviorChooser : public IBehaviorChooser
{
public:
  // constructor/destructor
  StrictPriorityBehaviorChooser(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  virtual ~StrictPriorityBehaviorChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual ICozmoBehaviorPtr GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior) override;

  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
private:
  std::vector<ICozmoBehaviorPtr> _behaviors;
};
   
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_StrictPriorityBehaviorChooser_H__
