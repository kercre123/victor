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

#include "engine/aiComponent/behaviorComponent/activities/activities/activityStrictPriority.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/activities/activityStrategies/iActivityStrategy.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/robot.h"

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "util/helpers/boundedWhile.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kActivitiesConfigKey = "subActivities";
static const char* activityIDKey        = "behaviorID";
static const char* activityPriorityKey  = "activityPriority";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrictPriority::ActivityStrictPriority(const Json::Value& config)
: IActivity(config)
, _currentActivityPtr(nullptr)
, _behaviorWait(nullptr)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityStrictPriority::InitActivity(BehaviorExternalInterface& behaviorExternalInterface)
{
  // TODO: rename _activities
  _activities.clear();
  
  // read every activity and create them with their own config
  const Json::Value& activityPriorities = _config[kActivitiesConfigKey];
  if ( !activityPriorities.isNull() )
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    const auto& activityData = robot.GetContext()->GetDataLoader()->GetBehaviorJsons();
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
      
      BehaviorID activityID = BehaviorIDFromString(activityID_str);
      
      const uint activityPriority = JsonTools::ParseUint8(*it, activityPriorityKey,
                                                          "ActivityStrictPriority.CreateFromConfig.ActivityPriority");
      
      DEV_ASSERT_MSG(activityPriority >= debugLastActivityPriority,
                     "ActivityStrictPriority.CreateFromConfig.ActivityPrioritiesOutOfOrder",
                     "Activity id %s", BehaviorIDToString(activityID));
      debugLastActivityPriority = activityPriority;
      
      
      
      // Locate the file data by its file name
      const auto& dataIter = activityData.find(activityID);
      
      DEV_ASSERT_MSG(dataIter != activityData.end(),
                     "ActivityStrictPriority.CreateFromConfig.ActivityDataNotFound",
                     "No file found named %s", BehaviorIDToString(activityID));
      
      // Gather the appropriate data from the config to generate the activity
      const Json::Value& activityConfig = dataIter->second;
      
      BehaviorID id = ICozmoBehavior::ExtractBehaviorIDFromConfig(activityConfig);
      ICozmoBehaviorPtr subActivity = behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(id);
      
      _activities.emplace_back(std::static_pointer_cast<IActivity>(subActivity));
    }
  }
  
  // DebugPrintActivities();
  
  // clear current activity
  _currentActivityPtr = nullptr;
  _behaviorWait = behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(BehaviorID::Wait);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityStrictPriority::OnActivatedActivity(BehaviorExternalInterface& behaviorExternalInterface)
{
  BehaviorUpdate(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr ActivityStrictPriority::GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior)
{
  ICozmoBehaviorPtr desiredActiveBehavior;
  BOUNDED_WHILE(5, true)
  {
    // Check to see if the current
    if(_currentActivityPtr != nullptr){
      const bool activityWantsToEnd = _currentActivityPtr->GetStrategy().WantsToEnd(
                                          behaviorExternalInterface,
                                          _currentActivityPtr->GetLastTimeStartedSecs());
      if(!activityWantsToEnd){
        desiredActiveBehavior = _currentActivityPtr->GetDesiredActiveBehavior(behaviorExternalInterface, currentRunningBehavior);
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
      if((*activityIter)->GetStrategy().WantsToStart(behaviorExternalInterface, lastTimeFinished, lastTimeStarted)){
        _currentActivityPtr = (*activityIter).get();
        break;
      }
    }
    DEV_ASSERT(activityIter != _activities.end(),
               "ActivityStrictPriority.GetDesiredActiveBehaviorInternal.NoActivatableActivities");
    
    // if this is the last activity, break without checking WantsToENd
    if(_currentActivityPtr == _activities.back().get()){
      break;
    }
  }
  
  return _currentActivityPtr->GetDesiredActiveBehavior(behaviorExternalInterface, currentRunningBehavior);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityStrictPriority::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for(auto activityIter = _activities.begin(); activityIter != _activities.end(); ++activityIter){
    (*activityIter)->GetAllDelegates(delegates);
  }
  IActivity::GetAllDelegates(delegates);
  delegates.insert(_behaviorWait.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityStrictPriority::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(USE_BSM &&
     behaviorExternalInterface.HasDelegationComponent()){
    auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();
    if(delegationComponent.IsControlDelegated(this)){
      ICozmoBehaviorPtr nextBehavior = GetDesiredActiveBehavior(behaviorExternalInterface, nullptr);
      // For activity to be "sticky" for legacy reasons we always need to delegate to something
      if(nextBehavior == nullptr){
        nextBehavior = _behaviorWait;
        _behaviorWait->WantsToBeActivated(behaviorExternalInterface);
      }
      
      if((nextBehavior != nullptr) &&
         delegationComponent.HasDelegator(this)){
        delegationComponent.GetDelegator(this).Delegate(this, nextBehavior.get());
      }
    }
  }else{
    IActivity::BehaviorUpdate(behaviorExternalInterface);
  }
}

  
  
} // namespace Cozmo
} // namespace Anki
