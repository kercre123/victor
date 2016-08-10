/**
 * File: AIGoal
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: High level goal: a group of behaviors that make sense to run together towards a common objective
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoal_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoal_H__

#include "json/json-forwards.h"
#include "clad/types/unlockTypes.h"

#include <cassert>
#include <memory>

namespace Anki {
namespace Cozmo {

class IAIGoalStrategy;
class IBehaviorChooser;
class IBehavior;
class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIGoal
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AIGoal
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  AIGoal();
  ~AIGoal();

  // initialize a goal with the given config. Return true on success, false if config is not valids
  bool Init(Robot& robot, const Json::Value& config);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Goal switch
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // when goal is selected
  void Enter();
  
  // when goal is kicked out or finishes
  void Exit();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behaviors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // choose next behavior for this goal
  IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns sparkId required to run this goal, UnlockId::Count if none required
  UnlockId GetRequiredSpark() const { return _requiredSpark; }
  
  // returns priority to compare against goals that want to run at the same time
  uint8_t GetPriority() const { return _priority; }
  
  // return strategy that defines this goal's selection
  const IAIGoalStrategy& GetStrategy() const { assert(_strategy); return *_strategy.get(); }
  
  // returns goal name set from config
  const std::string& GetName() const { return _name; }
  
  float GetLastTimeStartedSecs() const { return _lastTimeGoalStartedSecs; }
  float GetLastTimeStoppedSecs() const { return _lastTimeGoalStoppedSecs; }

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Constants and types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // behavior chooser associated to this goal
  std::unique_ptr<IBehaviorChooser> _behaviorChooserPtr;
  
  // strategy to run this goal
  std::unique_ptr<IAIGoalStrategy> _strategy;
  
  // priority against other goals available at the same time (from config)
  uint8_t _priority;
  
  // goal name (from config)
  std::string _name;
  
  // spark required for this goal
  UnlockId _requiredSpark;
  
  // last time the goal started running
  float _lastTimeGoalStartedSecs;
  // last time the goal stopped running
  float _lastTimeGoalStoppedSecs;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
