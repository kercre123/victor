/**
 * File: AIGoalEvaluator
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Evaluator/Selector of high level goals/objectives for the robot
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoalEvaluator_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoalEvaluator_H__

#include "iBehaviorChooser.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface_fwd.h"
#include "util/helpers/templateHelpers.h"
#include "json/json-forwards.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Anki {
namespace Cozmo {

class AIGoal;
class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIGoalEvaluator
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AIGoalEvaluator : public IBehaviorChooser
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor/destructor
  AIGoalEvaluator(Robot& robot, const Json::Value& config);
  ~AIGoalEvaluator();
  
  // Delegates to current goal
  virtual Result Update(Robot& robot) override;
  
  // exit the current goal so that we clean up any conditions we've set up before entering the new chooser
  virtual void OnDeselected() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behavior selection
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // calculate what the goal should be given the recently detected objects (blocks, faces, ...)
  void CalculateDesiredGoalFromObjects();

  // get next behavior by properly managing the goals
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Acccessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // name (for debug/identification)
  virtual const char* GetName() const override { return _name.c_str(); }
  
  // sets the name of a goal we want to force for debugging (not to be used in production)
  void SetConsoleRequestedGoalName(const std::string& name) { _debugConsoleRequestedGoal = name; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
  template<typename T> void HandleMessage(const T& msg);
  
  void SwitchToObjectTapInteractionGoal() { _requestedGoal = _configParams.objectTapInteractionGoalName; }
  void ClearObjectTapInteractionRequestedGoal();
  
  bool IsCurrentGoalObjectTapInteraction() const;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  struct Configuration {
    // goals we want to select for recent objects
    std::string faceAndCubeGoalName;
    std::string faceOnlyGoalName;
    std::string cubeOnlyGoalName;
    std::string noFaceNoCubeGoalName;
    
    std::string objectTapInteractionGoalName;
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // create the proper goals from the given config
  void CreateFromConfig(Robot& robot, const Json::Value& config);
  
  // picks a new goal from the available ones for the given spark. Note that at the moment it will only switch
  // between 2 per spark, since it grabs the first available one that is not the current one
  // returns true if current goal changes, false otherwise
  bool PickNewGoalForSpark(Robot& robot, UnlockId spark, bool isCurrentAllowed);
  
  // just prints loaded goals to log
  void DebugPrintGoals() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // jsong config params
  Configuration _configParams;
  
  // name for debugging
  std::string _name;
  
  // set of goals defined for this evaluator, stored by spark required to run the goal
  using GoalVector = std::vector< std::unique_ptr<AIGoal>>;
  using SparkToGoalsTable = std::unordered_map<UnlockId, GoalVector, Util::EnumHasher>;
  SparkToGoalsTable _goals;
  
  // raw pointer to the current goal, which is guaranteed be stored in _goals
  AIGoal* _currentGoalPtr;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // this goal is requested by external systems under certain circumstances
  std::string _requestedGoal;
  
  // this variable is set from debug console to cycle through goals in goal evaluators
  std::string _debugConsoleRequestedGoal;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
