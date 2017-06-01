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


#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface_fwd.h"
#include "json/json-forwards.h"
#include "util/helpers/templateHelpers.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Anki {
namespace Cozmo {

class IActivity;
class Robot;

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
  ActivityFreeplay(Robot& robot, const Json::Value& config);
  ~ActivityFreeplay();
  
  
  // Delegates to current activity
  virtual Result Update(Robot& robot) override;
  
  // exit the current activity so that we clean up any conditions we've set up before entering the new chooser
  virtual void OnDeselectedInternal() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behavior selection
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // calculate what the activity should be given the recently detected objects (blocks, faces, ...)
  void CalculateDesiredActivityFromObjects();

  // get next behavior by properly managing the sub-activities
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Acccessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // sets the name of an activity we want to force for debugging (not to be used in production)
  void SetConsoleRequestedActivity(ActivityID activityID) { _debugConsoleRequestedActivity = activityID; }
  
  // So that the behavior manager can check whether it should switch to the
  // object tap interaction activity
  std::vector<IBehavior*> GetObjectTapBehaviors();
  
  void SetActivityStrategyCooldown(const UnlockId& unlockID,
                                   const ActivityID& activityId,
                                   float cooldown_ms);
  
  using ActivityVector = std::vector< std::unique_ptr<IActivity>>;
  using SparkToActivitiesTable = std::unordered_map<UnlockId, ActivityVector, Util::EnumHasher>;
  
  const SparkToActivitiesTable& GetFreeplayActivities() const { return _activities; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
  template<typename T> void HandleMessage(const T& msg);
  
  void SwitchToObjectTapInteractionActivity() { _requestedActivity = _configParams.objectTapInteractionActivity; }
  void ClearObjectTapInteractionRequestedActivity();
  
  bool IsCurrentActivityObjectTapInteraction() const;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  struct Configuration {
    // activities we want to select for recent objects
    ActivityID faceAndCubeActivity;
    ActivityID faceOnlyActivity;
    ActivityID cubeOnlyActivity;
    ActivityID noFaceNoCubeActivity;
    ActivityID objectTapInteractionActivity;
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // create the proper activities from the given config
  void CreateFromConfig(Robot& robot, const Json::Value& config);
  
  // picks a new activity from the available ones for the given spark. Note that at the moment it will only switch
  // between 2 per spark, since it grabs the first available one that is not the current one
  // returns true if current activity changes, false otherwise
  bool PickNewActivityForSpark(Robot& robot, UnlockId spark, bool isCurrentAllowed);
  
  // just prints loaded activites to log
  void DebugPrintActivities() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // jsong config params
  Configuration _configParams;
  
  // set of activities defined for this evaluator, stored by spark required to run the activity
  SparkToActivitiesTable _activities;
  
  // raw pointer to the current activity, which is guaranteed be stored in _activities
  IActivity* _currentActivityPtr;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // this activity is requested by external systems under certain circumstances
  ActivityID _requestedActivity;
  
  // this variable is set from debug console to cycle through activities in activity evaluators
  ActivityID _debugConsoleRequestedActivity;
  
  Robot& _robot;
  
  // cache of behaviors in the objectTapInteraction activity
  std::vector<IBehavior*> _objectTapBehaviorsCache;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFreeplay_H__
