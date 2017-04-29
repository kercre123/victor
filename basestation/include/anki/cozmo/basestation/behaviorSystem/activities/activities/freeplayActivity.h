/**
 * File: freeplayActivity.h
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
#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_FreeplayActivity_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_FreeplayActivity_H__


#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface_fwd.h"
#include "util/helpers/templateHelpers.h"
#include "json/json-forwards.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Anki {
namespace Cozmo {

class IActivity;
class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// FreeplayActivity
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class FreeplayActivity : public IBehaviorChooser
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor/destructor
  FreeplayActivity(Robot& robot, const Json::Value& config);
  ~FreeplayActivity();
  
  // Delegates to current activity
  virtual Result Update(Robot& robot) override;
  
  // exit the current activity so that we clean up any conditions we've set up before entering the new chooser
  virtual void OnDeselected() override;

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

  // name (for debug/identification)
  virtual const char* GetName() const override { return _name.c_str(); }
  
  // sets the name of an activity we want to force for debugging (not to be used in production)
  void SetConsoleRequestedActivityName(const std::string& name) { _debugConsoleRequestedActivity = name; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
  template<typename T> void HandleMessage(const T& msg);
  
  void SwitchToObjectTapInteractionActivity() { _requestedActivity = _configParams.objectTapInteractionActivityName; }
  void ClearObjectTapInteractionRequestedActivity();
  
  bool IsCurrentActivityObjectTapInteraction() const;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  struct Configuration {
    // activities we want to select for recent objects
    std::string faceAndCubeActivityName;
    std::string faceOnlyActivityName;
    std::string cubeOnlyActivityName;
    std::string noFaceNoCubeActivityName;
    std::string objectTapInteractionActivityName;
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
  
  // name for debugging
  std::string _name;
  
  // set of activities defined for this evaluator, stored by spark required to run the activity
  using ActivityVector = std::vector< std::unique_ptr<IActivity>>;
  using SparkToActivitiesTable = std::unordered_map<UnlockId, ActivityVector, Util::EnumHasher>;
  SparkToActivitiesTable _activities;
  
  // raw pointer to the current activity, which is guaranteed be stored in _activities
  IActivity* _currentActivityPtr;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // this activity is requested by external systems under certain circumstances
  std::string _requestedActivity;
  
  // this variable is set from debug console to cycle through activities in activity evaluators
  std::string _debugConsoleRequestedActivity;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_FreeplayActivity_H__
