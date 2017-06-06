/**
* File: strictPriorityBehaviorChooser.cpp
*
* Author: Kevin M. Karol
* Created: 05/18/2017
*
* Description: Class for handling picking of behaviors strictly based on their priority
*
* Copyright: Anki, Inc. 2017
*
**/
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/strictPriorityBehaviorChooser.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kBehaviorsInChooserConfigKey     = "behaviors";

}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrictPriorityBehaviorChooser::StrictPriorityBehaviorChooser(Robot& robot, const Json::Value& config)
:IBehaviorChooser(robot, config)
{
  const Json::Value& behaviorArray = config[kBehaviorsInChooserConfigKey];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "StrictPriorityBehaviorChooser.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()){
    const BehaviorFactory& behaviorFactory = robot.GetBehaviorFactory();
    
    for(const auto& behaviorIDStr: behaviorArray)
    {
      BehaviorID behaviorID = BehaviorIDFromString(behaviorIDStr.asString());
      
      IBehavior* behavior =  behaviorFactory.FindBehaviorByID(behaviorID);
      DEV_ASSERT_MSG(behavior != nullptr,
                     "ScoringBehaviorChooser.ReloadFromConfig.FailedToFindBehavior",
                     "Behavior not found: %s",
                     BehaviorIDToString(behaviorID));
      if(behavior != nullptr){
        _behaviors.push_back(behavior);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrictPriorityBehaviorChooser::~StrictPriorityBehaviorChooser()
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* StrictPriorityBehaviorChooser::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  BehaviorPreReqRobot preReq(robot);
  // Iterate through available behaviors, and return the first one that is runnable
  // since this is the highest priority behavior
  for(const auto& entry: _behaviors){
    if(entry->IsRunning() || entry->IsRunnable(preReq)){
      return entry;
    }
  }
  
  return nullptr;
}

  
} // namespace Cozmo
} // namespace Anki
