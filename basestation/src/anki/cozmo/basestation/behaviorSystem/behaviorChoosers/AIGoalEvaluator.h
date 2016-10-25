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

#include "json/json-forwards.h"
#include "iBehaviorChooser.h"
#include "util/helpers/templateHelpers.h"

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
  virtual Result Update() override;
  
  // exit the current goal so that we clean up any conditions we've set up before entering the new chooser
  virtual void OnDeselected() override;
  
  // read which groups/behaviors are enabled/disabled from json configuration
  virtual void ReadEnabledBehaviorsConfiguration(const Json::Value& inJson) override { assert(false); } // todo rsam probably read goals, not behavior groups/behaviors
  virtual std::vector<std::string> GetEnabledBehaviorList()  override {std::vector<std::string> list; return list;};

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behavior selection
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // get next behavior by properly managing the goals
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Acccessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // name (for debug/identification)
  virtual const char* GetName() const override { return _name.c_str(); }
  
  // sets the name of a goal we want to force for debugging (not to be used in production)
  void SetConsoleRequestedGoalName(const std::string& name) { _debugConsoleRequestedGoal = name; }
  
private:

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
  
  // name for debugging
  std::string _name;
  
  // set of goals defined for this evaluator, stored by spark required to run the goal
  using GoalVector = std::vector< std::unique_ptr<AIGoal>>;
  using SparkToGoalsTable = std::unordered_map<UnlockId, GoalVector, Util::EnumHasher>;
  SparkToGoalsTable _goals;
  
  // raw pointer to the current goal, which is guaranteed be stored in _goals
  AIGoal* _currentGoalPtr;
  
  // this variable is set from debug console to cycle through goals in goal evaluators
  std::string _debugConsoleRequestedGoal;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
