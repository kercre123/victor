/**
 * File: ActivityFreeplay.h
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: High level activity which determines which freeplay subActivities 
 * to run
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFreeplay_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFreeplay_H__


#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include "engine/externalInterface/externalInterface_fwd.h"
#include "json/json-forwards.h"
#include "util/helpers/templateHelpers.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorExternalInterface;
class IActivity;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ActivityFreeplay
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ActivityFreeplay : public IActivity
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor/destructor
  ActivityFreeplay(const Json::Value& config);
  ~ActivityFreeplay();
  
  
  // Delegates to current activity
  virtual Result Update_Legacy(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // exit the current activity so that we clean up any conditions we've set up before entering the new chooser
  virtual void OnDeactivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behavior selection
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // calculate what the activity should be given the recently detected objects (blocks, faces, ...)
  void CalculateDesiredActivityFromObjects(BehaviorExternalInterface& behaviorExternalInterface);

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Acccessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // sets the name of an activity we want to force for debugging (not to be used in production)
  void SetConsoleRequestedActivity(BehaviorID activityID) { _debugConsoleRequestedActivity = activityID; }
  
  void SetActivityStrategyCooldown(const UnlockId& unlockID,
                                   const BehaviorID& activityId,
                                   float cooldown_ms);
  
  using ActivityVector = std::vector<std::shared_ptr<IActivity>>;
  using SparkToActivitiesTable = std::unordered_map<UnlockId, ActivityVector, Util::EnumHasher>;
  
  const SparkToActivitiesTable& GetFreeplayActivities() const { return _activities; }

protected:

  // get next behavior by properly managing the sub-activities
  virtual ICozmoBehaviorPtr GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior) override;
  
  virtual void InitActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  struct Configuration {
    // activities we want to select for recent objects
    BehaviorID faceAndCubeActivity;
    BehaviorID faceOnlyActivity;
    BehaviorID cubeOnlyActivity;
    BehaviorID noFaceNoCubeActivity;
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // create the proper activities from the given config
  void CreateFromConfig(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  
  // picks a new activity from the available ones for the given spark. Note that at the moment it will only switch
  // between 2 per spark, since it grabs the first available one that is not the current one
  // returns true if current activity changes, false otherwise
  bool PickNewActivityForSpark(BehaviorExternalInterface& behaviorExternalInterface, UnlockId spark, bool isCurrentAllowed);
  
  // just prints loaded activites to log
  void DebugPrintActivities() const;
  
  // Allows activities to pass up a display name from sub activities
  void SetActivityIDFromSubActivity(BehaviorID activityID);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // jsong config params
  Configuration _configParams;
  
  // set of activities defined for this evaluator, stored by spark required to run the activity
  SparkToActivitiesTable _activities;
  
  // External interface for freeplay to pass in with messages - should be removed
  // when messaging is passed in syncronously 	COZMO-14461
  BehaviorExternalInterface* _behaviorExternalInterface;
  
  // raw pointer to the current activity, which is guaranteed be stored in _activities
  IActivity* _currentActivityPtr;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // this activity is requested by external systems under certain circumstances
  BehaviorID _requestedActivity;
  
  // this variable is set from debug console to cycle through activities in activity evaluators
  BehaviorID _debugConsoleRequestedActivity;
  
  BehaviorID _subID;
  // Maintain the constructed freeplayIDString
  std::string _freeplayIDString;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFreeplay_H__
