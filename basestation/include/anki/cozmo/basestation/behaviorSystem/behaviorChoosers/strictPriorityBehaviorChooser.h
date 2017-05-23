/**
* File: strictPriorityBehaviorChooser.h
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

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {
  
class IBehavior;
  
class StrictPriorityBehaviorChooser : public IBehaviorChooser
{
public:
  // constructor/destructor
  StrictPriorityBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~StrictPriorityBehaviorChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;

private:
  std::vector<IBehavior*> _behaviors;
};
   
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_StrictPriorityBehaviorChooser_H__
