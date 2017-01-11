/**
 * File: behaviorChooser.h
 *
 * Author: Lee
 * Created: 08/20/15, raul 05/03/16
 *
 * Description: Class for handling picking of behaviors.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_SimpleBehaviorChooser_H__
#define __Cozmo_Basestation_SimpleBehaviorChooser_H__

#include "iBehaviorChooser.h"
#include "anki/common/types.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupFlags.h"
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
struct ScoredBehaviorInfo;
template <typename Type> class AnkiEvent;
 
// A simple implementation for choosing behaviors based on score only
class SimpleBehaviorChooser : public IBehaviorChooser
{
public:

  // constructor/destructor
  SimpleBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~SimpleBehaviorChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  // name (for debug/identification)
  virtual const char* GetName() const override { return "Simple"; }

  // read which groups/behaviors are enabled/disabled from json configuration
  virtual void ReadEnabledBehaviorsConfiguration(const Json::Value& inJson) override;
  virtual std::vector<std::string> GetEnabledBehaviorList()  override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds a specific behavior to the table of behaviors if name doesn't exist, otherwise fail
  Result TryAddBehavior(IBehavior* behavior);
  
  // returns true if the given behavior is enabled, false if disabled
  bool IsBehaviorEnabled(const std::string& name) const;
  
protected:  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // initialize the chooser, return result of operation
  Result ReloadFromConfig(Robot& robot, const Json::Value& config);
  
  // remove all behaviors currently in this chooser
  void ClearBehaviors();
  
  // add behaviors based on groups defined in config
  void AddFactoryBehaviorsFromGroupConfig(Robot& robot, const Json::Value& groupList);
  
  // enable/disable groups based on config lists
  void SetBehaviorEnabledFromGroupConfig(const Json::Value& groupList, bool enable);        // group list
  void SetBehaviorEnabledFromBehaviorConfig(const Json::Value& behaviorList, bool enable);  // behavior name list
  
  // internally change which behaviors can be selected
  void SetAllBehaviorsEnabled(bool newVal = true);
  void SetBehaviorGroupEnabled(BehaviorGroup behaviorGroup, bool newVal = true);
  bool SetBehaviorEnabled(const std::string& behaviorName, bool newVal = true);
  
  // returns a pointer to the behavior in our table whose key matches the passed name, nullptr if not found
  IBehavior* FindBehaviorInTableByName(const std::string& name);
  
  float ScoreBonusForCurrentBehavior(float runningDuration) const;

  // Allows derived classes to modify a score for a particular behavior. Updates passed in score value (if desired)
  virtual void ModifyScore(const IBehavior* behavior, float& score) const {}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  using NameToScoredBehaviorInfoMap = std::map<std::string, ScoredBehaviorInfo>;
  NameToScoredBehaviorInfoMap _nameToScoredBehaviorInfoMap;
  
  Util::GraphEvaluator2d  _scoreBonusForCurrentBehavior;
  
  IBehavior* _behaviorNone = nullptr;
};
   
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_SimpleBehaviorChooser_H__
