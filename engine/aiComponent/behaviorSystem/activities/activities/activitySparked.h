/**
* File: ActivitySparked.h
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Activity for handeling cozmo's "sparked" mode
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivitySparked_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivitySparked_H__

#include "engine/aiComponent/behaviorSystem/activities/activities/iActivity.h"

#include "anki/common/basestation/objectIDs.h"

#include "engine/components/bodyLightComponentTypes.h"
#include "clad/types/behaviorSystem/behaviorObjectives.h"
#include "util/signals/simpleSignal.hpp"

namespace Json {
class Value;
}


namespace Anki {
namespace Cozmo {

//Forward declarations
class IBehavior;
class MoodManager;
class Robot;
class BehaviorExternalInterface;
class BehaviorPeekABoo;
class BehaviorPlayArbitraryAnim;
class BehaviorAcknowledgeObject;
template <typename Type> class AnkiEvent;

class ActivitySparked : public IActivity
{
public:
  ActivitySparked(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  ~ActivitySparked();
  

  virtual Result Update_Legacy(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

protected:
  virtual IBehaviorPtr GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior) override;

  virtual void OnActivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnDeactivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // initialize the chooser, return result of operation
  Result ReloadFromConfig(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  
private:
  
  void CheckIfSparkShouldEnd(BehaviorExternalInterface& behaviorExternalInterface);
  void CompleteSparkLogic(BehaviorExternalInterface& behaviorExternalInterface);
  void ResetLightsAndAnimations(BehaviorExternalInterface& behaviorExternalInterface);

  enum class ChooserState{
    ChooserSelected,
    PlayingSparksIntro,
    UsingSimpleBehaviorChooser,
    WaitingForCurrentBehaviorToStop,
    PlayingSparksOutro,
    EndSparkWhenReactionEnds
  };
    
  ChooserState _state;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // Created with factory
  std::shared_ptr<BehaviorPlayArbitraryAnim> _behaviorPlayAnimation;
  // To clear objects to be acknowledged when sparked
  std::shared_ptr<BehaviorAcknowledgeObject> _behaviorAcknowledgeObject;
  
  // Reset each time spark is activated
  float _timeChooserStarted;
  int _currentObjectiveCompletedCount;
  
  // Loaded in from behavior_config
  float _minTimeSecs;
  float _maxTimeSecs;
  // Indicates whether max timeout should indicate ending "politely" at the action
  // level or the behavior level
  bool _maxTimeoutForActionComplete;
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
  std::unique_ptr<IActivity> _subActivityDelegate;
  
  BackpackLightDataLocator  _bodyLightDataLocator{};
  
  // for COZMO-8914 - peek a boo grabbed directly so that it can know when
  // a spark starts and when it has to play its get out;
  std::shared_ptr<BehaviorPeekABoo> _behaviorPeekABoo;
  
  IBehaviorPtr _behaviorWait = nullptr;
  
  IBehaviorPtr SelectNextSparkInternalBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior);

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivitySparked_H__
