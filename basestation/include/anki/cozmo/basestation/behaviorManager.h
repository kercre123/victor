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

#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"

#include "clad/types/activityTypes.h"
#include "clad/types/behaviorTypes.h"
#include "clad/types/unlockTypes.h"

#include "util/helpers/templateHelpers.h"
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
class BehaviorFactory;
class IActivity;
class IBehavior;
class IReactionTriggerStrategy;
class Robot;
struct BehaviorRunningAndResumeInfo;
struct TriggerBehaviorInfo;
  
template<typename TYPE> class AnkiEvent;

namespace Audio {
class BehaviorAudioClient;
}
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
  
  BehaviorManager(Robot& robot);
  ~BehaviorManager();

  // initialize this behavior manager from the given Json config
  Result InitConfiguration(const Json::Value& activitiesConfig);
  void LoadBehaviorsIntoFactory();
  Result InitReactionTriggerMap(const Json::Value& config);
  
  // create a behavior from the given config and add to the factory. Can fail if the configuration is not valid
  Result CreateBehaviorFromConfiguration(const Json::Value& behaviorConfig);

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Games
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // get if the given flags are available, you can check for all or if any is set
  inline bool AreAllGameFlagsAvailable(BehaviorGameFlag gameFlag) const;
  inline bool IsAnyGameFlagAvailable(BehaviorGameFlag gameFlag) const;
  
  UnlockId GetActiveSpark() const { return _activeSpark; }
  UnlockId GetRequestedSpark() const { return _lastRequestedSpark; }
  
  bool IsActiveSparkSoft() const { return (_activeSpark != UnlockId::Count) && _isSoftSpark; }
  bool IsActiveSparkHard() const { return (_activeSpark != UnlockId::Count) && !_isSoftSpark; }
  
  bool IsRequestedSparkSoft() const { return (_lastRequestedSpark != UnlockId::Count) && _isRequestedSparkSoft; }
  bool IsRequestedSparkHard() const { return (_lastRequestedSpark != UnlockId::Count) && !_isRequestedSparkSoft; }
  
  // sets which games are available by setting the mask/flag combination
  void SetAvailableGame(BehaviorGameFlag availableGames) { _availableGames = Util::EnumToUnderlying(availableGames); }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Calls the current behavior's Update() method until it returns COMPLETE or FAILURE.
  Result Update(Robot& robot);
        
  // Set the current Activity. Note this results in the destruction of the current Activity
  // (if there is one) and the IBehaviors it contains
  // ignorePreviousActivity is used during behaviorManager setup to indicate that there
  // is no previous high level activity that needs to be stopped - aviods a crash
  void SetCurrentActivity(HighLevelActivity newActivity, const bool ignorePreviousActivity = false);
  
  // returns last time we changed behavior chooser
  float GetLastBehaviorChooserSwitchTime() const { return _lastChooserSwitchTime; }

  // Stops the current behavior and switches to null. The next time Update is called, the behavior chooser
  // will have a chance to select a new behavior. This is mostly useful as a hack to force a behavior switch
  // when it is needed (e.g. sparks). `stoppedByWhom` is a string for debugging so we know why this was stopped
  void RequestCurrentBehaviorEndImmediately(const std::string& stoppedByWhom);
  
  // returns nullptr if there is no current behavior
  const IBehavior* GetCurrentBehavior() const;
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
  
  const BehaviorFactory& GetBehaviorFactory() const { assert(_behaviorFactory); return *_behaviorFactory; }
        BehaviorFactory& GetBehaviorFactory()       { assert(_behaviorFactory); return *_behaviorFactory; }

  // accessors: audioController
  Audio::BehaviorAudioClient& GetAudioClient() const { assert(_audioClient); return *_audioClient;}
  
  void HandleMessage(const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message);
  

  void RequestCurrentBehaviorEndOnNextActionComplete();
  
  // Have behavior manager maintain light state on blocks
  void SetBehaviorStateLights(BehaviorClass classSettingLights, const std::vector<BehaviorStateLightInfo>& structToSet, bool persistOnReaction);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Freeplay - specific
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // calculates (and sets in the freeplay chooser) the desired activity due to objects recently seen
  void CalculateActivityFreeplayFromObjects();

  // return the basestation time that freeplay first started (often useful as a notion of "session"). This
  // will be -1 until freeplay has started, and then will always be set to the time (in seconds) that freeplay
  // started
  float GetFirstTimeFreeplayStarted() const { return _firstTimeFreeplayStarted; }
  
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
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ObjectTapInteractions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Handles switching behavior chooser, activity, and updating whiteboard for object tap interactions
  void HandleObjectTapInteraction(const ObjectID& objectID);
  
  // Leave object interaction state resets the activity and clears the tap-intended object in the whiteboard
  void LeaveObjectTapInteraction();
  
  const ObjectID& GetLastTappedObject() const { return _lastDoubleTappedObject; }
  const ObjectID& GetCurrTappedObject() const { return _currDoubleTappedObject; }
  
  void OnRobotDelocalized();
  
private:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InitializeEventHandlers();
  
  // switches to a given behavior (stopping the current behavior if necessary). Returns true if it switched
  bool SwitchToReactionTrigger(IReactionTriggerStrategy& triggerStrategy, IBehavior* nextBehavior);
  bool SwitchToBehaviorBase(BehaviorRunningAndResumeInfo& nextBehaviorInfo);
  bool SwitchToVoiceCommandBehavior(IBehavior* nextBehavior);
  
  // checks the chooser and switches to a new behavior if neccesary
  void ChooseNextScoredBehaviorAndSwitch();
  
  // try to resume back to the previous behavior (after interruptions). This method does not guarantee resuming
  // to a previous one. If there was no behavior to resume to, it will set null behavior as current.
  void TryToResumeBehavior();

  // stop the current behavior if it is non-null and running (i.e. Init was called)
  void StopAndNullifyCurrentBehavior();
  
  // Allow reactionary behaviors to request a switch without a message
  bool CheckReactionTriggerStrategies();
  
  // Centeralized function for robot properties that are toggled between reactions
  // and normal robot opperation
  void UpdateRobotPropertiesForReaction(bool enablingReaction);

  void SendDasTransitionMessage(const BehaviorRunningAndResumeInfo& oldBehaviorInfo,
                                const BehaviorRunningAndResumeInfo& newBehaviorInfo);
  
  // broadcasting the reaction trigger map (to the game)
  void BroadcastReactionTriggerMap() const;
  
  // Allow unity to set head angles and lift heights to preserve after reactionary behaviors
  void SetDefaultHeadAndLiftState(bool enable, f32 headAngle, f32 liftHeight);
  bool AreDefaultHeadAndLiftStateSet() { return _defaultHeadAngle != kIgnoreDefaultHeadAndLiftState;}
  
  // update the tapped object should its pose change
  void UpdateTappedObject();
  
  // update current behavior with the new tapped object
  void UpdateBehaviorWithObjectTapInteraction();
  
  // Functions which mediate direct access to running/resume info so that the robot
  // can respond appropriately to switching between reactions/resumes
  BehaviorRunningAndResumeInfo& GetRunningAndResumeInfo() const {assert(_runningAndResumeInfo); return *_runningAndResumeInfo;}
  void SetRunningAndResumeInfo(const BehaviorRunningAndResumeInfo& newInfo);
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  bool _isInitialized = false;
    
  Robot& _robot;
  
  // Set by unity to preserve the head and lift angle after reactionary behaviors
  f32 _defaultHeadAngle;
  f32 _defaultLiftHeight;
  
  // - - - - - - - - - - - - - - -
  // current running behavior
  // - - - - - - - - - - - - - - -
  
  // PLEASE DO NOT ACCESS OR ASSIGN TO DIRECTLY - use functions above
  std::unique_ptr<BehaviorRunningAndResumeInfo> _runningAndResumeInfo;

  // name of activity in _highLevelActivityMap which is currently active
  HighLevelActivity _currentHighLevelActivity;
  
  // Maps activity types
  std::map<HighLevelActivity, std::shared_ptr<IActivity>> _highLevelActivityMap;
  
  // - - - - - - - - - - - - - - -
  // factory & choosers
  // - - - - - - - - - - - - - - -
  
  // Factory creates and tracks data-driven behaviors etc
  BehaviorFactory* _behaviorFactory = nullptr;
  
  // map of reactionTriggers to the associated strategies/behaviors
  // that fire as reactions to events
  std::map<ReactionTrigger,TriggerBehaviorInfo> _reactionTriggerMap;
  
  // time at which last chooser was selected
  float _lastChooserSwitchTime;

  // first time freeplay ever started
  float _firstTimeFreeplayStarted = -1.0f;

  // - - - - - - - - - - - - - - -
  // others/shared
  // - - - - - - - - - - - - - - -
  
  // games that are available currently for Cozmo to request
  using BehaviorGameFlagMask = std::underlying_type<BehaviorGameFlag>::type;
  BehaviorGameFlagMask _availableGames = Util::EnumToUnderlying( BehaviorGameFlag::NoGame );
  
  // current active spark (this does not guarantee that behaviors will kick in, only that Cozmo is in a Sparked state)
  UnlockId _activeSpark = UnlockId::Count;
  
  // Identifies if the spark is a "soft spark" - played when upgrade is unlocked and has some different playback features than normal sparks
  bool _isSoftSpark = false;
  
  // holding variables to keep track of most recent spark messages from game
  // until ActivityFreeplay switches out the active spark
  UnlockId _lastRequestedSpark = UnlockId::Count;
  bool _isRequestedSparkSoft = false;
  bool _didGameRequestSparkEnd = false;
  
  // Behavior audio client is used to update the audio engine with the current sparked state (a.k.a. "round")
  std::unique_ptr<Audio::BehaviorAudioClient> _audioClient;
      
  // For storing event handlers
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  // The last object that was double tapped used for clearing object tap interaction lights
  ObjectID _lastDoubleTappedObject;
  ObjectID _currDoubleTappedObject;
  
  // The object that was just double tapped, will become current double tapped object on next Update()
  ObjectID _pendingDoubleTappedObject;
  
  // Whether or not we need to handle an object being tapped in Update()
  bool _needToHandleObjectTapped = false;
  
  std::vector<BehaviorStateLightInfo> _behaviorStateLights;
  BehaviorClass _behaviorThatSetLights;
  bool _behaviorStateLightsPersistOnReaction;
}; // class BehaviorManager

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Inline
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::AreAllGameFlagsAvailable(BehaviorGameFlag gameFlag) const
{
  const BehaviorGameFlagMask flags = Util::EnumToUnderlying(gameFlag);
  const bool allSet = ((_availableGames & flags) == flags);
  return allSet;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::IsAnyGameFlagAvailable(BehaviorGameFlag gameFlag) const
{
  const BehaviorGameFlagMask flags = Util::EnumToUnderlying(gameFlag);
  const bool anySet = ((_availableGames & flags) != 0);
  return anySet;
}

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_MANAGER_H
