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
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/common/types.h"
#include "clad/types/activityTypes.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/unlockTypes.h"
#include "json/json-forwards.h"

#include <cassert>
#include <functional>
#include <memory>
#include <set>

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
  bool SupportsObjectTapInteractions() { return _supportsObjectTapInteractions;}


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Behaviors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // choose next behavior for this activity
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior);
  
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
  const char* GetIDStr() const { return ActivityIDToString(_id); }
  
  float GetLastTimeStartedSecs() const { return _lastTimeActivityStartedSecs; }
  float GetLastTimeStoppedSecs() const { return _lastTimeActivityStoppedSecs; }
  
  // Used to access objectTapInteraction behaviors
  std::vector<IBehavior*> GetObjectTapBehaviors();
  
protected:
  using TriggersArray = ReactionTriggerHelpers::FullReactionArray;
  
  virtual void OnSelectedInternal() {};
  virtual void OnDeselectedInternal() {};
  // Allows activities to pass up a display name from sub activities
  void SetActivityIDFromSubActivity(ActivityID activityID){ _id = activityID;}
  
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

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  

  
  // strategy to run this activity
  std::unique_ptr<IActivityStrategy> _strategy;
  
  // behavior chooser for activity (if one is passed in)
  std::unique_ptr<IBehaviorChooser> _behaviorChooserPtr;
  
  // activity name - defined in config or passed up from sub-activity
  ActivityID _id;
  
  // optional driving animations associated to this activity
  AnimationTrigger _driveStartAnimTrigger;
  AnimationTrigger _driveLoopAnimTrigger;
  AnimationTrigger _driveEndAnimTrigger;
  
  // optional AIInformationAnalyzer process
  AIInformationAnalysis::EProcess _infoAnalysisProcess;
  
  // spark required for this activity
  UnlockId _requiredSpark;
  
  bool _requireObjectTapped = false;

  bool _supportsObjectTapInteractions = false;
  
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
