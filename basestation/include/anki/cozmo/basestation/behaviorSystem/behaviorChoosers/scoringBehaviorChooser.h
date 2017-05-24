/**
* File: scoringBehaviorChooser.h
*
* Author: Lee
* Created: 08/20/15, raul 05/03/16
*
* Description: Class for handling picking of behaviors strictly based on their
* current score
*
* Copyright: Anki, Inc. 2015
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_ScoringBehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_ScoringBehaviorChooser_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/common/types.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/helpers/noncopyable.h"
#include <map>
#include <string>
#include <set>
#include <vector>
#include <functional>


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {
  
//Forward declarations
class IBehavior;
class MoodManager;
class Robot;
template <typename Type> class AnkiEvent;
 
// A simple implementation for choosing behaviors based on score only
class ScoringBehaviorChooser : public IBehaviorChooser
{
public:
  // constructor/destructor
  ScoringBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~ScoringBehaviorChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds a specific behavior to the table of behaviors if name doesn't exist, otherwise fail
  Result TryAddBehavior(IBehavior* behavior);
  
  
  // Used to access objectTapInteraction behaviors
  std::vector<IBehavior*> GetObjectTapBehaviors();
  
protected:  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // initialize the chooser, return result of operation
  Result ReloadFromConfig(Robot& robot, const Json::Value& config);
  
  // remove all behaviors currently in this chooser
  void ClearBehaviors();
  
  // returns a pointer to the behavior in our table whose key matches the passed name, nullptr if not found
  IBehavior* FindBehaviorInTableByID(BehaviorID behaviorID);
  
  float ScoreBonusForCurrentBehavior(float runningDuration) const;

  // Allows derived classes to modify a score for a particular behavior. Updates passed in score value (if desired)
  virtual void ModifyScore(const IBehavior* behavior, float& score) const {}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  using BehaviorIDToScoredBehaviorInfoMap = std::map<BehaviorID, IBehavior*>;
  BehaviorIDToScoredBehaviorInfoMap _idToScoredBehaviorMap;
  
  Util::GraphEvaluator2d  _scoreBonusForCurrentBehavior;
  
  IBehavior* _behaviorNone = nullptr;
};
   
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_ScoringBehaviorChooser_H__
