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

#include <memory>
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

  // read which groups/behaviors are enabled/disabled from json configuration
  virtual void ReadEnabledBehaviorsConfiguration(const Json::Value& inJson) override { assert(false); } // todo rsam probably read goals, not behavior groups/behaviors

  // get next behavior by properly managing the goals
  virtual IBehavior* ChooseNextBehavior(const Robot& robot) const override { return nullptr; } // todo rsam: ask current goal

  // name (for debug/identification)
  // todo rsam: add current goal name to the name (like Demo does)
  virtual const char* GetName() const override { return "GoalEvaluator"; }
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Constants and Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // json configuration
  static const char* kSelfConfigKey;
  static const char* kGoalsConfigKey;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // create the proper goals from the given config
  void CreateFromConfig(Robot& robot, const Json::Value& config);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set of goals defined for this evaluator
  std::vector< std::unique_ptr<AIGoal> > _goals;

};
  

} // namespace Cozmo
} // namespace Anki

#endif //
