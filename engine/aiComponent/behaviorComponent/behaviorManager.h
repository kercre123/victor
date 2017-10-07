/**
 * File: behaviorManager.h
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 *
 * Overhaul: 7/30/15 Andrew Stein, 2016-04-18 Brad Neuman
 *
 * Description: High-level module that is a container for available behaviors,
 *              determines what behavior a robot should be executing at a given
 *              time and handles ticking the running behavior forward.
 *
 * Copyright: Anki, Inc. 2014-2016
 **/

#ifndef COZMO_BEHAVIOR_MANAGER_H
#define COZMO_BEHAVIOR_MANAGER_H

#include "anki/common/types.h"
#include "anki/common/basestation/objectIDs.h"

#include "engine/aiComponent/behaviorComponent/behaviors/ICozmoBehavior_fwd.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/robotDataLoader.h"

#include "clad/types/behaviorSystem/activityTypes.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "clad/types/unlockTypes.h"

#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"

#include "json/json-forwards.h"

#include <assert.h>
#include <list>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <float.h>


namespace Anki {
namespace Cozmo {
  
// Forward declaration
class BehaviorContainer;
class BehaviorExternalInterface;
class BehaviorRequestGameSimple;
class IActivity;
class IReactionTriggerStrategy;
class Robot;
struct BehaviorRunningAndResumeInfo;
struct TriggerBehaviorInfo;
  
template<typename TYPE> class AnkiEvent;

namespace ExternalInterface {
class BehaviorManagerMessageUnion;
}

static const f32 kIgnoreDefaultHeadAndLiftState = FLT_MAX;
  
  
// struct for mapping light info to specific cubes during behavior state
struct BehaviorStateLightInfo{
public:
  BehaviorStateLightInfo(const ObjectID& objectID,
                         const CubeAnimationTrigger& animTrigger,
                         bool hasModifier = false,
                         const ObjectLights& modifier = ObjectLights{})
  : _objectID(objectID)
  , _animTrigger(animTrigger)
  , _hasModifier(hasModifier)
  , _modifier(modifier)
  {}
  
  const ObjectID& GetObjectID() const { return _objectID;}
  CubeAnimationTrigger GetAnimationTrigger() const { return _animTrigger; }
  bool GetLightModifier(ObjectLights& lights) const {
    if(_hasModifier) { lights = _modifier; return true;}
    else{ return false;}
  };
    
private:
  ObjectID _objectID;
  CubeAnimationTrigger _animTrigger;
  bool _hasModifier;
  ObjectLights _modifier;
};
  
  
class BehaviorManager
{
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  BehaviorManager();
  ~BehaviorManager();

  // initialize this behavior manager from the given Json config
  Result InitConfiguration(BehaviorExternalInterface& behaviorExternalInterface,
                           const Json::Value& activitiesConfig);
  Result InitReactionTriggerMap(BehaviorExternalInterface& behaviorExternalInterface,
                                const Json::Value& config);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Games
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  UnlockId GetActiveSpark() const { return _activeSpark; }
  UnlockId GetRequestedSpark() const { return _lastRequestedSpark; }
  
  bool IsActiveSparkSoft() const { return (_activeSpark != UnlockId::Count) && _isSoftSpark; }
  bool IsActiveSparkHard() const { return (_activeSpark != UnlockId::Count) && !_isSoftSpark; }
  
  bool IsRequestedSparkSoft() const { return (_lastRequestedSpark != UnlockId::Count) && _isRequestedSparkSoft; }
  bool IsRequestedSparkHard() const { return (_lastRequestedSpark != UnlockId::Count) && !_isRequestedSparkSoft; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Calls the current behavior's Update() method until it returns COMPLETE or FAILURE.
  Result Update(BehaviorExternalInterface& behaviorExternalInterface);
        
  // Set the current Activity. Note this results in the destruction of the current Activity
  // (if there is one) and the ICozmoBehaviors it contains
  // ignorePreviousActivity is used during behaviorManager setup to indicate that there
  // is no previous high level activity that needs to be stopped - aviods a crash
  void SetCurrentActivity(BehaviorExternalInterface& behaviorExternalInterface,
                          HighLevelActivity newActivity, const bool ignorePreviousActivity = false);
  
  // returns last time we changed behavior chooser
  float GetLastBehaviorChooserSwitchTime() const { return _lastChooserSwitchTime; }

  // Stops the current behavior and switches to null. The next time Update is called, the behavior chooser
  // will have a chance to select a new behavior. This is mostly useful as a hack to force a behavior switch
  // when it is needed (e.g. sparks). `stoppedByWhom` is a string for debugging so we know why this was stopped
  void RequestCurrentBehaviorEndImmediately(const std::string& stoppedByWhom);
  
  // returns nullptr if there is no current behavior
  const ICozmoBehaviorPtr GetCurrentBehavior() const;
  bool CurrentBehaviorTriggeredAsReaction() const;
  ReactionTrigger GetCurrentReactionTrigger() const;

  // returns true if the current activity is not null
  std::shared_ptr<IActivity> GetCurrentActivity();
  
  using TriggersArray = ReactionTriggerHelpers::FullReactionArray;
  
  // Disables all triggers passed in through AllTriggersConsidered by placing
  // a lock with the specified lockID on those triggers.  The specified reactions
  // will always be disabled after this function call
  //
  // StopCurrent specifies whether in the case that a behavior
  // triggered by a reaction is currently running,
  // and that trigger is now disabled, the behavior should be stopped
  void DisableReactionsWithLock(
             const std::string& lockID,
             const ReactionTriggerHelpers::FullReactionArray& triggersAffected,
             bool stopCurrent = true);
  
  // Removes the reaction lock from all triggers disabled by the lock
  // this does not gaurentee that reactions are re-enabled as there
  // may be additional disable locks still present on them
  void RemoveDisableReactionsLock(const std::string& lockID);
  
#if ANKI_DEV_CHEATS
  void DisableReactionWithLock(const std::string& lockID,
                               const ReactionTrigger& trigger,
                               bool stopCurrent = true);
#endif
  
  // Allows other parts of the system to determine whether a reaction is enabled
  bool IsReactionTriggerEnabled(ReactionTrigger reaction) const;
  
  ICozmoBehaviorPtr FindBehaviorByID(BehaviorID behaviorID) const;
  ICozmoBehaviorPtr FindBehaviorByExecutableType(ExecutableBehaviorType type) const;

  // TODO:(bn) automatically infer requiredClass from T
  
  void HandleMessage(BehaviorExternalInterface& behaviorExternalInterface,
                     const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message);
  

  void RequestCurrentBehaviorEndOnNextActionComplete();
  
  // Have behavior manager maintain light state on blocks
  void SetBehaviorStateLights(BehaviorClass classSettingLights, const std::vector<BehaviorStateLightInfo>& structToSet, bool persistOnReaction);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Freeplay - specific
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // calculates (and sets in the freeplay chooser) the desired activity due to objects recently seen
  void CalculateActivityFreeplayFromObjects(BehaviorExternalInterface& behaviorExternalInterface);

  // return the basestation time that freeplay first started (often useful as a notion of "session"). This
  // will be -1 until freeplay has started, and then will always be set to the time (in seconds) that freeplay
  // started
  float GetFirstTimeFreeplayStarted() const { return _firstTimeFreeplayStarted; }
  
  // Returns all the non spark related freeplay activities
  const std::list<const IActivity* const> GetNonSparkFreeplayActivities() const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Sparks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Allows ActivityFreeplay to know if it should kick out the current activity and transition into a spark
  // ActivityFreeplay only switches from non-sparked to sparked.  Otherwise the sparkBehaviorChooser will kill itself
  bool ShouldSwitchToSpark() const { return _activeSpark == UnlockId::Count && _activeSpark != _lastRequestedSpark;}
  
  // Actually switches out the LastRequestedSpark from the game with the active spark
  // Returns the UnlockID of the newly active spark
  const UnlockId SwitchToRequestedSpark();
  
  // Allows sparks to know if the game has requested that they quit when able
  bool DidGameRequestSparkEnd() const{ return _didGameRequestSparkEnd;}
  
  // Stores the requested spark until it (or a subsequent request) is swapped in to ActiveSpark with a call to
  // SwitchToRequestedSpark.  This simplifies asyncronous messaging issues
  // and gives sparks the opportunity to end themselves if they are canceled by the user
  void SetRequestedSpark(UnlockId spark, bool softSpark);
  
  void OnRobotDelocalized();
  
  const BehaviorContainer& GetBehaviorContainer();

private:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InitializeEventHandlers(BehaviorExternalInterface& behaviorExternalInterface);
  
  // switches to a given behavior (stopping the current behavior if necessary). Returns true if it switched
  bool SwitchToReactionTrigger(BehaviorExternalInterface& behaviorExternalInterface,
                               IReactionTriggerStrategy& triggerStrategy, ICozmoBehaviorPtr nextBehavior);
  bool SwitchToBehaviorBase(BehaviorExternalInterface& behaviorExternalInterface, BehaviorRunningAndResumeInfo& nextBehaviorInfo);
  bool SwitchToVoiceCommandBehavior(BehaviorExternalInterface& behaviorExternalInterface,
                                    ICozmoBehaviorPtr nextBehavior);
  void SwitchToUIGameRequestBehavior(BehaviorExternalInterface& behaviorExternalInterface);
  
  // checks the chooser and switches to a new behavior if neccesary
  void ChooseNextScoredBehaviorAndSwitch(BehaviorExternalInterface& behaviorExternalInterface);
  
  // try to resume back to the previous behavior (after interruptions). This method does not guarantee resuming
  // to a previous one. If there was no behavior to resume to, it will set null behavior as current.
  void TryToResumeBehavior(BehaviorExternalInterface& behaviorExternalInterface);

  // stop the current behavior if it is non-null and running (i.e. Init was called)
  void StopAndNullifyCurrentBehavior(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Called at the Complete or Failed state of a behavior in order to switch to a new one
  void FinishCurrentBehavior(BehaviorExternalInterface& behaviorExternalInterface,
                             ICozmoBehaviorPtr activeBehavior, bool shouldAttemptResume);
  
  // Allow reactionary behaviors to request a switch without a message
  bool CheckReactionTriggerStrategies(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Centeralized function for robot properties that are toggled between reactions
  // and normal robot opperation
  void UpdateRobotPropertiesForReaction(BehaviorExternalInterface& behaviorExternalInterface,
                                        bool enablingReaction);

  void SendDasTransitionMessage(BehaviorExternalInterface& behaviorExternalInterface,
                                const BehaviorRunningAndResumeInfo& oldBehaviorInfo,
                                const BehaviorRunningAndResumeInfo& newBehaviorInfo);
  
  // broadcasting the reaction trigger map (to the game)
  void BroadcastReactionTriggerMap(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  // Allow unity to set head angles and lift heights to preserve after reactionary behaviors
  void SetDefaultHeadAndLiftState(BehaviorExternalInterface& behaviorExternalInterface,
                                  bool enable, f32 headAngle, f32 liftHeight);
  bool AreDefaultHeadAndLiftStateSet() { return _defaultHeadAngle != kIgnoreDefaultHeadAndLiftState;}
  
  // Functions which mediate direct access to running/resume info so that the robot
  // can respond appropriately to switching between reactions/resumes
  BehaviorRunningAndResumeInfo& GetRunningAndResumeInfo() const {assert(_runningAndResumeInfo); return *_runningAndResumeInfo;}
  void SetRunningAndResumeInfo(BehaviorExternalInterface& behaviorExternalInterface,
                               const BehaviorRunningAndResumeInfo& newInfo);
  
  // Selects a game request behavior to run
  void SelectUIRequestGameBehavior(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Clears flags and attributes related to running a ui-driven game request
  void EnsureRequestGameIsClear(BehaviorExternalInterface& behaviorExternalInterface);

  // helper to avoid including ICozmoBehavior.h here
  BehaviorClass GetBehaviorClass(ICozmoBehaviorPtr behavior) const;
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  BehaviorExternalInterface* _behaviorExternalInterface = nullptr;
  
  bool _isInitialized = false;
      
  // Set by unity to preserve the head and lift angle after reactionary behaviors
  f32 _defaultHeadAngle;
  f32 _defaultLiftHeight;
  
  // Maps UnlockIds to the appropriate request behavior to play when the ui requests it
  std::map<UnlockId, std::shared_ptr<BehaviorRequestGameSimple>> _uiGameRequestMap;
  
  // - - - - - - - - - - - - - - -
  // current running behavior
  // - - - - - - - - - - - - - - -
  
  // PLEASE DO NOT ACCESS OR ASSIGN TO DIRECTLY - use functions above
  std::unique_ptr<BehaviorRunningAndResumeInfo> _runningAndResumeInfo;

  // name of activity in _highLevelActivityMap which is currently active
  HighLevelActivity _currentHighLevelActivity;
  
  // Maps activity types
  std::map<HighLevelActivity, std::shared_ptr<IActivity>> _highLevelActivityMap;
  
  // Pointer to current request game behavior driven by UI, if any
  std::shared_ptr<BehaviorRequestGameSimple> _uiRequestGameBehavior;
  bool _shouldRequestGame = false;
  
  // map of reactionTriggers to the associated strategies/behaviors
  // that fire as reactions to events
  std::map<ReactionTrigger,TriggerBehaviorInfo> _reactionTriggerMap;
  
  // Don't react to anything until at least one action has been queued
  // or the SDK has requested that reactions become enabled
  bool _hasActionQueuedOrSDKReactEnabled;
  
  // time at which last chooser was selected
  float _lastChooserSwitchTime;

  // first time freeplay ever started
  float _firstTimeFreeplayStarted = -1.0f;

  // - - - - - - - - - - - - - - -
  // others/shared
  // - - - - - - - - - - - - - - -
  
  // current active spark (this does not guarantee that behaviors will kick in, only that Cozmo is in a Sparked state)
  UnlockId _activeSpark = UnlockId::Count;
  
  // Identifies if the spark is a "soft spark" - played when upgrade is unlocked and has some different playback features than normal sparks
  bool _isSoftSpark = false;
  
  // holding variables to keep track of most recent spark messages from game
  // until ActivityFreeplay switches out the active spark
  UnlockId _lastRequestedSpark = UnlockId::Count;
  bool _isRequestedSparkSoft = false;
  bool _didGameRequestSparkEnd = false;
      
  // For storing event handlers
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  std::vector<BehaviorStateLightInfo> _behaviorStateLights;
  BehaviorClass _behaviorThatSetLights;
  bool _behaviorStateLightsPersistOnReaction;
}; // class BehaviorManager

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_MANAGER_H
