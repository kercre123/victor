/**
 * File: sparksBehaviorChooser.h
 *
 * Author: Kevin M. Karol
 * Created: 08/17/16
 *
 * Description: Class for handling playing sparks.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_SparksBehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_SparksBehaviorChooser_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/components/bodyLightComponentTypes.h"
#include "clad/types/behaviorObjectives.h"
#include "json/json-forwards.h"
#include "simpleBehaviorChooser.h"


namespace Anki {
namespace Cozmo {
  
//Forward declarations
class IBehavior;
class MoodManager;
class Robot;
class BehaviorPlayArbitraryAnim;
class BehaviorAcknowledgeObject;
template <typename Type> class AnkiEvent;
  
// A behavior chooser which handles soft sparking, intro/outro and relies on simple behavior chooser otherwise
class SparksBehaviorChooser : public SimpleBehaviorChooser
{
using BaseClass = SimpleBehaviorChooser;
  
public:

  // constructor/destructor
  SparksBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~SparksBehaviorChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result Update(Robot& robot) override;
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  // name (for debug/identification)
  virtual const char* GetName() const override { return "Sparks"; }
  
  virtual void OnSelected() override;
  virtual void OnDeselected() override;
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:
  // initialize the chooser, return result of operation
  Result ReloadFromConfig(Robot& robot, const Json::Value& config);

private:
  using TriggersArray = ReactionTriggerHelpers::FullReactionArray;

  void CheckIfSparkShouldEnd();
  void CompleteSparkLogic();
  void ResetLightsAndAnimations();
  void SmartDisableReactionsWithLock(const std::string& lockID,
                                     const TriggersArray& triggers);
  void SmartRemoveDisableReactionsLock(const std::string& lockID,
                                       const TriggersArray& triggers);
  
  enum class ChooserState{
    ChooserSelected,
    PlayingSparksIntro,
    UsingSimpleBehaviorChooser,
    WaitingForCurrentBehaviorToStop,
    PlayingSparksOutro,
    EndSparkWhenReactionEnds
  };
  
  Robot& _robot;
  
  ChooserState _state;
  std::vector<Signal::SmartHandle> _signalHandles;
  std::set<std::string> _smartLockIDs;

  // Created with factory
  BehaviorPlayArbitraryAnim* _behaviorPlayAnimation = nullptr;
  // To clear objects to be acknowledged when sparked
  BehaviorAcknowledgeObject* _behaviorAcknowledgeObject = nullptr;
  
  // Reset each time spark is activated
  float _timeChooserStarted;
  int _currentObjectiveCompletedCount;

  // Loaded in from behavior_config
  float _minTimeSecs;
  float _maxTimeSecs;
  // Setting numberOfRepetitions to 0 will cause the spark to always play until its max time
  // and then play the success animation.  This allows non-time dependent sparks
  int _numberOfRepetitions;
  BehaviorObjective _objectiveToListenFor;
  AnimationTrigger _softSparkUpgradeTrigger;
  AnimationTrigger _sparksSuccessTrigger;
  AnimationTrigger _sparksFailTrigger;

  // Special re-start indicator
  TimeStamp_t _timePlayingOutroStarted;
  bool _switchingToHardSpark;
  
  // Track if idle animations swapped out
  bool _idleAnimationsSet;

  // Track when we saw cubes to determine if we saw them during the spark
  std::set< ObjectID > _observedObjectsSinceStarted;
  
  // A behavior chooser that can be set by a spark to delegate selection
  // to once the intro has finished as part of the sparksChooser
  std::unique_ptr<IBehaviorChooser> _simpleBehaviorChooserDelegate;
  
  BackpackLightDataLocator  _bodyLightDataLocator{};
  
  IBehavior* SelectNextSparkInternalBehavior(Robot& robot, const IBehavior* currentRunningBehavior);

  
  
  
};
   
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_SparksBehaviorChooser_H__
