/**
 * File: IActivity
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Activities provide persistence of cozmo's character and game related
 * world state across behaviors.  They may set lights/music and select what behavior
 * should be running each tick
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_IActivity_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_IActivity_H__

#include "anki/cozmo/basestation/aiInformationAnalysis/aiInformationAnalysisProcessTypes.h"

#include "anki/common/types.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/unlockTypes.h"
#include "json/json-forwards.h"

#include <cassert>
#include <functional>
#include <memory>

namespace Anki {
namespace Cozmo {

class IActivityStrategy;
class IBehaviorChooser;
class IBehavior;
class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// IActivity
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class IActivity
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  IActivity();
  ~IActivity();

  // initialize an activity with the given config. Return true on success, false if config is not valids
  bool Init(Robot& robot, const Json::Value& config);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Activity switch
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // when activity is selected
  void Enter(Robot& robot);
  
  // when activity is kicked out or finishes
  void Exit(Robot& robot);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behaviors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // choose next behavior for this activity
  IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior);
  
  Result Update(Robot& robot);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns sparkId required to run this activity, UnlockId::Count if none required
  UnlockId GetRequiredSpark() const { return _requiredSpark; }
  
  // return strategy that defines this activity's selection
  const IActivityStrategy& GetStrategy() const { assert(_strategy); return *_strategy.get(); }
  
  // returns the activity name set from config
  const std::string& GetName() const { return _name; }
  
  float GetLastTimeStartedSecs() const { return _lastTimeActivityStartedSecs; }
  float GetLastTimeStoppedSecs() const { return _lastTimeActivityStoppedSecs; }

private:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Constants and types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns true if driving animation triggers have been defined for this activity
  bool HasDrivingAnimTriggers() const { return _driveStartAnimTrigger != AnimationTrigger::Count; } // checking one is checking all

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // behavior chooser associated with this activity
  std::unique_ptr<IBehaviorChooser> _behaviorChooserPtr;
  
  // strategy to run this activity
  std::unique_ptr<IActivityStrategy> _strategy;
  
  // activity name (from config)
  std::string _name;
  
  // optional driving animations associated to this activity
  AnimationTrigger _driveStartAnimTrigger;
  AnimationTrigger _driveLoopAnimTrigger;
  AnimationTrigger _driveEndAnimTrigger;
  
  // optional AIInformationAnalyzer process
  AIInformationAnalysis::EProcess _infoAnalysisProcess;
  
  // spark required for this activity
  UnlockId _requiredSpark;
  
  bool _requireObjectTapped = false;
  
  // last time the activity started running
  float _lastTimeActivityStartedSecs;
  // last time the activity stopped running
  float _lastTimeActivityStoppedSecs;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_IActivity_H__
