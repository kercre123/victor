/**
* File: ScoringBSRunnableChooser.h
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

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_ScoringBSRunnableChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_ScoringBSRunnableChooser_H__

#include "engine/aiComponent/behaviorSystem/bsRunnableChoosers/iBSRunnableChooser.h"
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
class ScoringBSRunnableChooser : public IBSRunnableChooser
{
public:
  // constructor/destructor
  ScoringBSRunnableChooser(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  virtual ~ScoringBSRunnableChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehaviorPtr GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior) override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds a specific behavior to the table of behaviors if name doesn't exist, otherwise fail
  Result TryAddBehavior(IBehaviorPtr behavior);
  
protected:  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // initialize the chooser, return result of operation
  Result ReloadFromConfig(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  
  // remove all behaviors currently in this chooser
  void ClearBehaviors();
  
  // returns a pointer to the behavior in our table whose key matches the passed name, nullptr if not found
  IBehaviorPtr FindBehaviorInTableByID(BehaviorID behaviorID);
  
  float ScoreBonusForCurrentBehavior(float runningDuration) const;

  // Allows derived classes to modify a score for a particular behavior. Updates passed in score value (if desired)
  virtual void ModifyScore(const IBehaviorPtr behavior, float& score) const {}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  using BehaviorIDToScoredBehaviorInfoMap = std::map<BehaviorID, IBehaviorPtr>;
  BehaviorIDToScoredBehaviorInfoMap _idToScoredBehaviorMap;
  
  Util::GraphEvaluator2d  _scoreBonusForCurrentBehavior;  
};
   
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_ScoringBSRunnableChooser_H__
