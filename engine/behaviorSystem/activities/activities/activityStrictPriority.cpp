/**
* File: activityStrictPriority.h
*
* Author: Kevin M. Karol
* Created: 08/22/17
*
* Description: Activity when choosing the next behavior is all that's desired
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/behaviorSystem/activities/activities/activityStrictPriority.h"
#include "engine/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/behaviorSystem/activities/activities/activityFactory.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/robot.h"

#include "util/helpers/boundedWhile.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kActivitiesConfigKey = "subActivities";
static const char* activityIDKey        = "activityID";
static const char* activityPriorityKey  = "activityPriority";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrictPriority::ActivityStrictPriority(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
, _currentActivityPtr(nullptr)
{
  
  // TODO: rename _activities
  _activities.clear();
  
  // read every activity and create them with their own config
  const Json::Value& activityPriorities = config[kActivitiesConfigKey];
  if ( !activityPriorities.isNull() )
  {
    const auto& activityData = robot.GetContext()->GetDataLoader()->GetActivityJsons();
    // Iterate through the activities
    uint debugLastActivityPriority = 0;
    
    // reserve room for activites (note we reserve 1 per activity as upper bound,
    //                             even if some activites fall in the same unlockId)
    _activities.reserve( activityPriorities.size() );
    
    // iterate all activity names in priority order, grab their data, and load them
    // into the container
    
    
    Json::Value::const_iterator it = activityPriorities.begin();
    const Json::Value::const_iterator end = activityPriorities.end();
    
    for(; it != end; ++it )
    {
      // Get the activityID and priority for the subActivity
      std::string activityID_str = JsonTools::ParseString(*it, activityIDKey,
                                                          "ActivityStrictPriority.CreateFromConfig.ActivityID.KeyMissing");
      
      ActivityID activityID = ActivityIDFromString(activityID_str);
      
      const uint activityPriority = JsonTools::ParseUint8(*it, activityPriorityKey,
                                                          "ActivityStrictPriority.CreateFromConfig.ActivityPriority");
      
      DEV_ASSERT_MSG(activityPriority >= debugLastActivityPriority,
                     "ActivityStrictPriority.CreateFromConfig.ActivityPrioritiesOutOfOrder",
                     "Activity id %s", ActivityIDToString(activityID));
      debugLastActivityPriority = activityPriority;
      
      
      
      // Locate the file data by its file name
      const auto& dataIter = activityData.find(activityID);
      
      DEV_ASSERT_MSG(dataIter != activityData.end(),
                     "ActivityStrictPriority.CreateFromConfig.ActivityDataNotFound",
                     "No file found named %s", ActivityIDToString(activityID));
      
      // Gather the appropriate data from the config to generate the activity
      const Json::Value& activityConfig = dataIter->second;
      
      ActivityType activityType =  IActivity::ExtractActivityTypeFromConfig(activityConfig);
      
      IActivity* subActivity = ActivityFactory::CreateActivity(robot,
                                                               activityType,
                                                               activityConfig);
      _activities.emplace_back(subActivity);
    }
  }
  
  // DebugPrintActivities();
  
  // clear current activity
  _currentActivityPtr = nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr ActivityStrictPriority::GetDesiredActiveBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior)
{
  IBehaviorPtr desiredActiveBehavior;
  BOUNDED_WHILE(5, true)
  {
    // Check to see if the current
    if(_currentActivityPtr != nullptr){
      const bool activityWantsToEnd = _currentActivityPtr->GetStrategy().WantsToEnd(
                                          robot,
                                          _currentActivityPtr->GetLastTimeStartedSecs());
      if(!activityWantsToEnd){
        desiredActiveBehavior = _currentActivityPtr->GetDesiredActiveBehavior(robot, currentRunningBehavior);
        if(desiredActiveBehavior != nullptr){
          return desiredActiveBehavior;
        }
      }
    }
    
    // if not, select a new activity as a strict priority
    ActivityVector::iterator activityIter;
    for(activityIter = _activities.begin(); activityIter != _activities.end(); ++activityIter){
      const float lastTimeStarted = (*activityIter)->GetLastTimeStartedSecs();
      const float lastTimeFinished = (*activityIter)->GetLastTimeStoppedSecs();
      if((*activityIter)->GetStrategy().WantsToStart(robot, lastTimeFinished, lastTimeStarted)){
        _currentActivityPtr = (*activityIter).get();
        break;
      }
    }
    DEV_ASSERT(activityIter != _activities.end(),
               "ActivityStrictPriority.GetDesiredActiveBehaviorInternal.NoRunnableActivities");
  }
  
  return _currentActivityPtr->GetDesiredActiveBehavior(robot, currentRunningBehavior);
}

  
  
} // namespace Cozmo
} // namespace Anki
