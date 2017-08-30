/**
* File: StrictPriorityBSRunnableChooser.h
*
* Author: Kevin M. Karol
* Created: 05/18/2017
*
* Description: Class for handling picking of behaviors strictly based on their priority
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_StrictPriorityBSRunnableChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_StrictPriorityBSRunnableChooser_H__

#include "engine/behaviorSystem/bsRunnableChoosers/iBSRunnableChooser.h"


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {
  
class IBehavior;
  
class StrictPriorityBSRunnableChooser : public IBSRunnableChooser
{
public:
  // constructor/destructor
  StrictPriorityBSRunnableChooser(Robot& robot, const Json::Value& config);
  virtual ~StrictPriorityBSRunnableChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehaviorPtr GetDesiredActiveBehavior(Robot& robot, const IBehaviorPtr currentRunningBehavior) override;

private:
  std::vector<IBehaviorPtr> _behaviors;
};
   
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_StrictPriorityBSRunnableChooser_H__
