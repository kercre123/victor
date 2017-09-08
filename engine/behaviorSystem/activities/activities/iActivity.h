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

#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalysisProcessTypes.h"
#include "engine/behaviorSystem/iBSRunnable.h"
#include "engine/behaviorSystem/behaviors/iBehavior_fwd.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"

#include "anki/common/types.h"
#include "clad/types/behaviorSystem/activityTypes.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/needsSystemTypes.h"
#include "clad/types/unlockTypes.h"
#include "json/json-forwards.h"

#include <cassert>
#include <functional>
#include <memory>
#include <set>

namespace Anki {
namespace Cozmo {

class IActivityStrategy;
class IBSRunnableChooser;
class Robot;
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// IActivity
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class IActivity : public IBSRunnable
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  IActivity(Robot& robot, const Json::Value& config);
  virtual ~IActivity();

  static ActivityID ExtractActivityIDFromConfig(const Json::Value& config,
                                                const std::string& fileName = "");
  static ActivityType ExtractActivityTypeFromConfig(const Json::Value& config);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Activity switch
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  void OnSelected(Robot& robot);
  void OnDeselected(Robot& robot);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behaviors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // choose next behavior for this activity
  IBehaviorPtr GetDesiredActiveBehavior(Robot& robot, const IBehaviorPtr currentRunningBehavior);
  
  virtual Result Update(Robot& robot) { return Result::RESULT_OK;}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns sparkId required to run this activity, UnlockId::Count if none required
  UnlockId GetRequiredSpark() const { return _requiredSpark; }
  
  // return strategy that defines this activity's selection
  const IActivityStrategy& GetStrategy() const { assert(_strategy); return *_strategy.get(); }
  IActivityStrategy* DevGetStrategy();
  
  // returns the activity name set from config
  ActivityID  GetID() const { return _id; }
  // Sub activities can override
  virtual const char* GetIDStr() const { return ActivityIDToString(_id); }
  
  float GetLastTimeStartedSecs() const { return _lastTimeActivityStartedSecs; }
  float GetLastTimeStoppedSecs() const { return _lastTimeActivityStoppedSecs; }
  
protected:
  // IBSRunnable methods - TO BE IMPLEMENTED - these will be used by the new BSM
  // as a uniform interface across Activities and Behaviors, but they will be
  // wired up in a seperate PR
  //virtual std::set<IBSRunnable> GetAllDelegates() override { return std::set<IBSRunnable>();}
  virtual void EnteredActivatableScopeInternal() override {};
  virtual BehaviorStatus UpdateInternal(Robot& robot) override { return BehaviorStatus::Complete;};
  virtual bool WantsToBeActivatedInternal() override { return false;};
  virtual void OnActivatedInternal() override {};
  virtual void OnDeactivatedInternal() override {};
  virtual void LeftActivatableScopeInternal() override {};
  
  using TriggersArray = ReactionTriggerHelpers::FullReactionArray;
  
  virtual void OnSelectedInternal(Robot& robot) {};
  virtual void OnDeselectedInternal(Robot& robot) {};

  // can be overridden by derived classes to chose behaviors. Defaults to using the config defined behavior chooser
  virtual IBehaviorPtr GetDesiredActiveBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior);
  
  // Push an idle animation which will be removed when the activity is deselected
  void SmartPushIdleAnimation(Robot& robot, AnimationTrigger animation);
  
  // Remove an idle animation before the activity is deselected
  void SmartRemoveIdleAnimation(Robot& robot);
  
  void SmartDisableReactionsWithLock(Robot& robot,
                                     const std::string& lockID,
                                     const TriggersArray& triggers);
  void SmartRemoveDisableReactionsLock(Robot& robot,
                                       const std::string& lockID);
  
  // Avoid calling this function directly, use the SMART_DISABLE_REACTION_DEV_ONLY macro instead
  // Locks a single reaction trigger instead of a full TriggersArray
#if ANKI_DEV_CHEATS
  void SmartDisableReactionWithLock(Robot& robot,
                                    const std::string& lockID,
                                    const ReactionTrigger& trigger);
#endif

  // Needs action ID that optionally gets registered with the needs manager
  NeedsActionId _needsActionId;

private:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Constants and types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns true if driving animation triggers have been defined for this activity
  bool HasDrivingAnimTriggers() const { return _driveStartAnimTrigger != AnimationTrigger::Count; } // checking one is checking all
  
  void ReadConfig(Robot& robot, const Json::Value& config);

  NeedsActionId ExtractNeedsActionIDFromConfig(const Json::Value& config);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  

  
  // strategy to run this activity
  std::unique_ptr<IActivityStrategy> _strategy;
  
  // behavior chooser for activity (if one is passed in). This is the default used if the derived class
  // doesn't override ChooseNextBehaviorInternal
  std::unique_ptr<IBSRunnableChooser> _behaviorChooserPtr;

  // Behavior chooser for interludes. An interlude behavior is one that runs in between two other behaviors
  // chosen by GetDesiredActiveBehaviorInternal(). It will get the behavior that is _about_ to run passed in as
  // currentRunningBehavior
  std::unique_ptr<IBSRunnableChooser> _interludeBehaviorChooserPtr;

  // The last chosen interlude behavior. When an interlude behavior is chosen, it is always allowed to run to
  // completion before another behavior gets selected
  IBehaviorPtr _lastChosenInterludeBehavior;
  
  // activity name - defined in config or passed up from sub-activity
  ActivityID _id;

  // optional driving animations associated to this activity
  AnimationTrigger _driveStartAnimTrigger;
  AnimationTrigger _driveLoopAnimTrigger;
  AnimationTrigger _driveEndAnimTrigger;

  // optional idle animation to be used during this activity
  AnimationTrigger _idleAnimTrigger;
  
  // optional AIInformationAnalyzer process
  AIInformationAnalysis::EProcess _infoAnalysisProcess;
  
  // spark required for this activity
  UnlockId _requiredSpark;
  
  // track whether the activity has set an idle
  bool _hasSetIdle;
  
  
  // last time the activity started running
  float _lastTimeActivityStartedSecs;
  // last time the activity stopped running
  float _lastTimeActivityStoppedSecs;
  
  // Track the locks which have been set through SmartDisableReactions
  std::set<std::string> _smartLockIDs;

};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_IActivity_H__
