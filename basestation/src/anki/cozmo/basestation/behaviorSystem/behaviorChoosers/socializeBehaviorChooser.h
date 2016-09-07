/**
 * File: socializeBehaviorChooser.h
 *
 * Author: Brad Neuman
 * Created: 2016-08-23
 *
 * Description: A behavior chooser to handle special logic for the AI "socialize" goal
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_SocializeBehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_SocializeBehaviorChooser_H__

#include "simpleBehaviorChooser.h"

#include "clad/types/behaviorObjectives.h"
#include "clad/types/unlockTypes.h"
#include "json/json-forwards.h"
#include "util/signals/simpleSignal_fwd.h"
#include <map>
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorExploreLookAroundInPlace;

// A helper class to handle objective requirements
struct ObjectiveRequirements {
  ObjectiveRequirements(const Json::Value& config);
  
  BehaviorObjective objective = BehaviorObjective::Count;
  UnlockId requiredUnlock = UnlockId::Count;
  float probabilityToRequire = 1.0f;
  unsigned int randCompletionsMin = 1;
  unsigned int randCompletionsMax = 1;
};

class FPSocializeBehaviorChooser : public SimpleBehaviorChooser
{
using BaseClass = SimpleBehaviorChooser;

public:

  FPSocializeBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~FPSocializeBehaviorChooser() {}

  // name (for debug/identification)
  virtual const char* GetName() const override { return "Socialize"; }

  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;

  // reset the state and populate the objective which we will require for this run (they are randomized each
  // time the goal is selected)
  virtual void OnSelected() override;

  template<typename T>
  void HandleMessage(const T& msg);

private:

  // use the objective requirements to populate _objectivesLeft, taking into account unlocks and random
  // probabilities.
  void PopulateRequiredObjectives();
  
  void PrintDebugObjectivesLeft(const std::string& eventName) const;

  enum class State {
    Initial,
    FindingFaces,
    Interacting,
    FinishedInteraction,
    Pouncing,
    FinishedPouncing,
    None
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Parameters set during init / construction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  BehaviorExploreLookAroundInPlace* _findFacesBehavior = nullptr;
  IBehavior* _interactWithFacesBehavior = nullptr;
  IBehavior* _pounceOnMotionBehavior = nullptr;

  unsigned int _maxNumIterationsToAllowForSearch = 0; // 0 means infinite

  // requirements defined from json
  using ObjectiveRequirementsList = std::vector< std::unique_ptr<ObjectiveRequirements> >;
  const ObjectiveRequirementsList _objectiveRequirements;

  // function to read requirements from json
  static ObjectiveRequirementsList ReadObjectiveRequirements(const Json::Value& config);  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Variables
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
  State _state;

  // keep track of the number of iterations FindFaces does, so we can stop it manually when we want to
  unsigned int _lastNumSearchIterations = 0;

  // keep track of the number of times pounce has started, so we can advance states as needed (to detect when
  // the pounce behavior has started and stopped)
  unsigned int _lastNumTimesPounceStarted = 0;

  // contains an entry for each objective we need to complete, mapping to the number of times we need to complete it
  std::map< BehaviorObjective, unsigned int > _objectivesLeft;

  std::vector<Signal::SmartHandle> _signalHandles;
};

}
}

#endif
