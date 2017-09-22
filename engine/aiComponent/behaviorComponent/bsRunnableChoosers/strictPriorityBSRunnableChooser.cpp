/**
* File: StrictPriorityBSRunnableChooser.cpp
*
* Author: Kevin M. Karol
* Created: 05/18/2017
*
* Description: Class for handling picking of behaviors strictly based on their priority
*
* Copyright: Anki, Inc. 2017
*
**/
#include "engine/aiComponent/behaviorComponent/bsRunnableChoosers/strictPriorityBSRunnableChooser.h"

#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"
#include "engine/robot.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

namespace{
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrictPriorityBSRunnableChooser::StrictPriorityBSRunnableChooser(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
:IBSRunnableChooser(behaviorExternalInterface, config)
{
  const char* kBehaviorsInChooserConfigKey     = "behaviors";
  const Json::Value& behaviorArray = config[kBehaviorsInChooserConfigKey];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "StrictPriorityBSRunnableChooser.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()){
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    const BehaviorManager& behaviorManager = robot.GetBehaviorManager();
    
    for(const auto& behaviorIDStr: behaviorArray)
    {
      BehaviorID behaviorID = BehaviorIDFromString(behaviorIDStr.asString());
      
      IBehaviorPtr behavior =  behaviorManager.FindBehaviorByID(behaviorID);
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
StrictPriorityBSRunnableChooser::~StrictPriorityBSRunnableChooser()
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr StrictPriorityBSRunnableChooser::GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior)
{
  // Iterate through available behaviors, and return the first one that is runnable
  // since this is the highest priority behavior
  for(const auto& entry: _behaviors){
    if(entry->IsRunning() || entry->WantsToBeActivated(behaviorExternalInterface)){
      return entry;
    }
  }
  
  return nullptr;
}

  
} // namespace Cozmo
} // namespace Anki
