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

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"

#include "anki/common/basestation/objectIDs.h"

#include "anki/cozmo/basestation/components/bodyLightComponentTypes.h"
#include "clad/types/behaviorObjectives.h"
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
class BehaviorPeekABoo;
class BehaviorPlayArbitraryAnim;
class BehaviorAcknowledgeObject;
template <typename Type> class AnkiEvent;

class ActivitySparked : public IActivity
{
public:
  ActivitySparked(Robot& robot, const Json::Value& config);
  ~ActivitySparked();
  
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  virtual Result Update(Robot& robot) override;
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

protected:
  virtual void OnSelectedInternal(Robot& robot) override;
  virtual void OnDeselectedInternal(Robot& robot) override;
  
  // initialize the chooser, return result of operation
  Result ReloadFromConfig(Robot& robot, const Json::Value& config);
  
private:
  
  void CheckIfSparkShouldEnd(Robot& robot);
  void CompleteSparkLogic(Robot& robot);
  void ResetLightsAndAnimations(Robot& robot);

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
  std::unique_ptr<IActivity> _subActivityDelegate;
  
  BackpackLightDataLocator  _bodyLightDataLocator{};
  
  // for COZMO-8914 - peek a boo grabbed directly so that it can know when
  // a spark starts and when it has to play its get out;
  BehaviorPeekABoo* _behaviorPeekABoo;
  
  IBehavior* _behaviorNone = nullptr;
  
  IBehavior* SelectNextSparkInternalBehavior(Robot& robot, const IBehavior* currentRunningBehavior);

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivitySparked_H__
