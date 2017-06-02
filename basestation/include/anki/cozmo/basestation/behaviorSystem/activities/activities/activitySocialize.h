/**
* File: ActivitySocialize.h
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Activity for cozmo to interact with the user's face
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivitySocialize_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivitySocialize_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"

#include "clad/types/behaviorTypes.h"
#include "clad/types/behaviorObjectives.h"
#include "util/signals/simpleSignal_fwd.h"
#include <map>
#include <vector>

namespace Json {
class Value;
}


namespace Anki {
namespace Cozmo {
  
class BehaviorExploreLookAroundInPlace;

// A helper class to handle objective requirements
struct PotentialObjectives {
  PotentialObjectives(const Json::Value& config);
  
  BehaviorObjective objective = BehaviorObjective::Count;
  BehaviorID behaviorID = BehaviorID::PounceOnMotion_Socialize;
  UnlockId requiredUnlock = UnlockId::Count;
  float probabilityToRequire = 1.0f;
  unsigned int randCompletionsMin = 1;
  unsigned int randCompletionsMax = 1;
};

class ActivitySocialize : public IActivity
{
public:
  ActivitySocialize(Robot& robot, const Json::Value& config);
  ~ActivitySocialize() {};
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:
  // reset the state and populate the objective which we will require for this run (they are randomized each
  // time the activity is selected)
  virtual void OnSelectedInternal(Robot& robot) override;
  
private:
  // use the objective requirements to populate _objectivesLeft, taking into account unlocks and random
  // probabilities.
  void PopulatePotentialObjectives();
  
  void PrintDebugObjectivesLeft(const std::string& eventName) const;
  
  enum class State {
    Initial,
    FindingFaces,
    Interacting,
    FinishedInteraction,
    Playing, // Either peekaboo or pouncing
    FinishedPlaying,
    None
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Parameters set during init / construction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  static constexpr IBehavior* _behaviorNone = nullptr;

  BehaviorExploreLookAroundInPlace* _findFacesBehavior = nullptr;
  IBehavior* _interactWithFacesBehavior = nullptr;
  
  IBehavior* _playingBehavior = nullptr;
  
  unsigned int _maxNumIterationsToAllowForSearch = 0; // 0 means infinite
  
  Robot& _robot;
  // requirements defined from json
  using PotentialObjectivesList = std::vector< std::unique_ptr<PotentialObjectives> >;
  const PotentialObjectivesList _potentialObjectives;
  
  // function to read requirements from json
  static PotentialObjectivesList ReadPotentialObjectives(const Json::Value& config);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Variables
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  State _state;
  
  // keep track of the number of iterations FindFaces does, so we can stop it manually when we want to
  unsigned int _lastNumSearchIterations = 0;
  
  // keep track of the number of times pounce has started, so we can advance states as needed (to detect when
  // the pounce behavior has started and stopped)
  unsigned int _lastNumTimesPlayStarted = 0;
  
  // contains an entry for each objective we need to complete, mapping to the number of times we need to complete it
  std::map< BehaviorObjective, unsigned int > _objectivesLeft;
  
  std::vector<Signal::SmartHandle> _signalHandles;
  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivitySocialize_H__
