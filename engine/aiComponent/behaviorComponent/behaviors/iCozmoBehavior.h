/**
 * File: ICozmoBehavior.h
 *
 * Author: Andrew Stein : Kevin M. Karol
 * Date:   7/30/15  : 12/1/16
 *
 * Description: Defines interface for a Cozmo "Behavior"
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Cozmo_Basestation_Behaviors_ICozmoBehavior_H__
#define __Cozmo_Basestation_Behaviors_ICozmoBehavior_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include "engine/actions/actionContainers.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalysisProcessTypes.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/helperHandle.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/moodSystem/moodScorer.h"
#include "engine/robotInterface/messageHandler.h"
#include <set>


#include "clad/types/actionResults.h"
#include "clad/types/behaviorComponent/behaviorObjectives.h"
#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "clad/types/needsSystemTypes.h"
#include "clad/types/needsSystemTypes.h"
#include "clad/types/behaviorComponent/reactionTriggers.h"
#include "clad/types/unlockTypes.h"
#include "util/logging/logging.h"

//Transforms enum into string
#define DEBUG_SET_STATE(s) SetDebugStateName(#s)

// Dev only macro to lock a single reaction trigger useful for enabling/disabling reactions
// with a console var
#if ANKI_DEV_CHEATS
  #define SMART_DISABLE_REACTION_DEV_ONLY(lock, trigger) SmartDisableReactionWithLock(lock, trigger)
#else
  #define SMART_DISABLE_REACTION_DEV_ONLY(lock, trigger)
#endif

namespace Anki {
namespace Util{
class RandomGenerator;
}
class ObjectID;
namespace Cozmo {
  
// Forward declarations
class Robot;
class ActionableObject;
class DriveToObjectAction;
class BehaviorHelperFactory;
class IHelper;
enum class ObjectInteractionIntention;
  
class ISubtaskListener;
class IReactToFaceListener;
class IReactToObjectListener;
class IReactToPetListener;
class IFistBumpListener;
class IFeedingListener;

enum class CubeAnimationTrigger;
struct BehaviorStateLightInfo;

struct PathMotionProfile;

namespace ExternalInterface {
struct BehaviorObjectiveAchieved;
}

// Base Behavior Interface specification
class ICozmoBehavior : public IBehavior
{
protected:  
  friend class BehaviorContainer;
  // Allows helpers to run DelegateIfInControl calls directly on the behavior that
  // delegated to them
  friend class IHelper;

  // Can't create a public ICozmoBehavior, but derived classes must pass a robot
  // reference into this protected constructor.
  ICozmoBehavior(const Json::Value& config);
    
  virtual ~ICozmoBehavior();
    
public:
  using Status = BehaviorStatus;
  
  static Json::Value CreateDefaultBehaviorConfig(BehaviorClass behaviorClass, BehaviorID behaviorID);
  static BehaviorID ExtractBehaviorIDFromConfig(const Json::Value& config, const std::string& fileName = "");
  static BehaviorClass ExtractBehaviorClassFromConfig(const Json::Value& config);

  bool IsActivated() const { return _isActivated; }

  // returns true if the behavior has delegated control to a helper/action/behavior
  bool IsControlDelegated() const;
  
  // returns true if any action from StartAction is currently running, indicating that the behavior is
  // likely waiting for something to complete
  bool IsActing() const;

  // returns the number of times this behavior has been started (number of times Init was called and returned
  // OK, not counting calls to Resume)
  int GetNumTimesBehaviorStarted() const { return _startCount; }
  void ResetStartCount() { _startCount = 0; }
  
  // Time this behavior was activated, in seconds, as measured by basestation time
  float GetTimeActivated_s() const { return _activatedTime_s; }
  
  // Time elapsed since start, in seconds, as measured by basestation time
  float GetActivatedDuration() const;
    
  // Will be called upon first switching to a behavior before calling update.
  // Calls protected virtual OnBehaviorActivated() method, which each derived class
  // should implement.
  void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override final
           {OnActivatedInternal_Legacy(behaviorExternalInterface); }
  Result OnActivatedInternal_Legacy(BehaviorExternalInterface& behaviorExternalInterface);

  // If this behavior is resuming after a short interruption (e.g. a cliff reaction), this Resume function
  // will be called instead. It calls ResumeInternal(), which defaults to simply calling OnBehaviorActivated, but can
  // be implemented by children to do specific resume behavior (e.g. start at a specific state). If this
  // function returns anything other than RESULT_OK, the behavior will not be resumed (but may still be Init'd
  // later)
  Result Resume(BehaviorExternalInterface& behaviorExternalInterface, ReactionTrigger resumingFromType);

  // Step through the behavior and deliver rewards to the robot along the way
  // This calls the protected virtual UpdateInternal() method, which each
  // derived class should implement.
  Status BehaviorUpdate_Legacy(BehaviorExternalInterface& behaviorExternalInterface);
    
  // This behavior was active, but is now stopping (to make way for a new current
  // behavior). Any behaviors from DelegateIfInControl will be canceled.
  void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;

  
  // Prevents the behavior from calling a callback function when ActionCompleted occurs
  // this allows the behavior to be stopped gracefully
  void StopOnNextActionComplete();
  
  // Returns true if the state of the world/robot is sufficient for this behavior to be executed
  bool WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const override final;

  BehaviorID         GetID()      const { return _id; }
  const std::string& GetIDStr()   const { return _idString; }

  void SetNeedsActionID(NeedsActionId needsActionID) { _needsActionID = needsActionID; }

  const std::string& GetDisplayNameKey() const { return _displayNameKey; }
  const std::string& GetDebugStateName() const { return _debugStateName;}
  ExecutableBehaviorType GetExecutableType() const { return _executableType; }
  const BehaviorClass GetClass() const { return _behaviorClassID; }

  // Can be overridden to allow the behavior to run while the robot is not on its treads (default is to not run)
  virtual bool ShouldRunWhileOffTreads() const { return false;}

  // Can be overridden to allow the behavior to run while the robot is on the charger platform
  virtual bool ShouldRunWhileOnCharger() const { return false;}

  // Return true if the behavior explicitly handles the case where the robot starts holding the block
  // Equivalent to !robot.IsCarryingObject() in WantsToBeActivated()
  virtual bool CarryingObjectHandledInternally() const = 0;

  // Return true if this is a good time to interrupt this behavior. This allows more gentle interruptions for
  // things which aren't immediately urgent. Eventually this may become a mandatory override, but for now, the
  // default is that we can never interrupt gently
  virtual bool CanBeGentlyInterruptedNow(BehaviorExternalInterface& behaviorExternalInterface) const { return false; }

  // Helper function for having DriveToObjectActions use the second closest preAction pose useful when the action
  // is being retried or the action failed due to visualVerification
  ActionResult UseSecondClosestPreActionPose(DriveToObjectAction* action,
                                             ActionableObject* object,
                                             std::vector<Pose3d>& possiblePoses,
                                             bool& alreadyInPosition);

  // returns required process
  AIInformationAnalysis::EProcess GetRequiredProcess() const { return _requiredProcess; }
  
  // returns the required unlockID for the behavior
  const UnlockId GetRequiredUnlockID() const {  return _requiredUnlockId;}

  // returns the need id of the severe need state that must be expressed (see AIWhiteboard), or Count if none
  NeedId GetRequiredSevereNeedExpression() const { return _requiredSevereNeed; }

  // Force a behavior to update its target blocks but only if it is in a state where it can
  void UpdateTargetBlocks(BehaviorExternalInterface& behaviorExternalInterface) const { UpdateTargetBlocksInternal(behaviorExternalInterface); }
  
  // Get the ObjectUseIntentions this behavior uses
  virtual std::set<ObjectInteractionIntention>
                           GetBehaviorObjectInteractionIntentions() const { return {}; }
  
  
  // Add Listeners to a behavior which will notify them of milestones/events in the behavior's lifecycle
  virtual void AddListener(ISubtaskListener* listener)
                { DEV_ASSERT(false, "AddListener.FrustrationListener.Unimplemented"); }
  virtual void AddListener(IReactToFaceListener* listener)
                { DEV_ASSERT(false, "AddListener.FaceListener.Unimplemented"); }
  virtual void AddListener(IReactToObjectListener* listener)
                { DEV_ASSERT(false, "AddListener.ObjectListener.Unimplemented"); }
  virtual void AddListener(IReactToPetListener* listener)
                { DEV_ASSERT(false, "AddListener.PetListener.Unimplemented"); }
  virtual void AddListener(IFistBumpListener* listener)
                { DEV_ASSERT(false, "AddListener.FistBumpListener.Unimplemented"); }
  virtual void AddListener(IFeedingListener* listener)
                { DEV_ASSERT(false, "AddListener.FeedingListener.Unimplemented"); }
  
protected:
  // Currently unused overrides of IBehavior since no equivalence in old BM system
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  
  using TriggersArray = ReactionTriggerHelpers::FullReactionArray;

  inline void SetDebugStateName(const std::string& inName) {
    PRINT_CH_INFO("Behaviors", "Behavior.TransitionToState", "Behavior:%s, FromState:%s ToState:%s",
                  BehaviorIDToString(GetID()), _debugStateName.c_str(), inName.c_str());
    _debugStateName = inName;
  }
  
  
  virtual void OnEnteredActivatableScopeInternal() override;
  virtual void OnLeftActivatableScopeInternal() override;

  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  virtual Result ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface);
  bool IsResuming() { return _isResuming;}

  
  void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override final;
  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) {};
  
  // To keep passing through data generic, if robot is not overridden
  // check the NoPreReqs WantsToBeActivatedBehavior
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const = 0;

  // This function can be implemented by behaviors. It should return Running while it is running, and Complete
  // or Failure as needed. If it returns Complete, Stop will be called. Default implementation is to
  // return Running while ControlIsDelegated, and Complete otherwise
  virtual void UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override final;
  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) {};
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface);
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) { };

  Util::RandomGenerator& GetRNG() const;
    
  // Derived classes should use these methods to subscribe to any tags they
  // are interested in handling.
  void SubscribeToTag(GameToEngineTag   tag, std::function<void(const GameToEngineEvent&)> messageHandler = nullptr);
  void SubscribeToTag(EngineToGameTag   tag, std::function<void(const EngineToGameEvent&)> messageHandler = nullptr);
  
  void SubscribeToTags(std::set<GameToEngineTag>&& tags);
  void SubscribeToTags(std::set<EngineToGameTag>&& tags);
  void SubscribeToTags(std::set<RobotInterface::RobotToEngineTag>&& tags);
  
  // Derived classes must override this method to handle events that come in
  // irrespective of whether the behavior is running or not. Note that the Robot
  // reference is const to prevent the behavior from modifying the robot when it
  // is not running. If the behavior is subscribed to multiple tags, the presumption
  // is that this will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called before HandleWhileRunning and HandleWhileNotRunning!
  virtual void AlwaysHandle(const GameToEngineEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) { }
  virtual void AlwaysHandle(const EngineToGameEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) { }
  virtual void AlwaysHandle(const RobotToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) { }
  
  // Derived classes must override this method to handle events that come in
  // while the behavior is running. In this case, the behavior is allowed to
  // modify the robot and thus receives a non-const reference to it.
  // If the behavior is subscribed to multiple tags, the presumption is that it
  // will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called first!
  virtual void HandleWhileActivated(const GameToEngineEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) { }
  virtual void HandleWhileActivated(const EngineToGameEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) { }
  virtual void HandleWhileActivated(const RobotToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) { }
  
  // Derived classes must override this method to handle events that come in
  // only while the behavior is NOT running. If it doesn't matter whether the
  // behavior is running or not, consider using AlwaysHandle above instead.
  // If the behavior is subscribed to multiple tags, the presumption is that it
  // will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called first!
  virtual void HandleWhileInScopeButNotActivated(const GameToEngineEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) { }
  virtual void HandleWhileInScopeButNotActivated(const EngineToGameEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) { }
  virtual void HandleWhileInScopeButNotActivated(const RobotToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) { }
  
  // Many behaviors use a pattern of executing an action, then waiting for it to finish before selecting the
  // next action. Instead of directly starting actions and handling ActionCompleted callbacks, derived
  // classes can use the functions below. Note: none of the DelegateIfInControl functions can be called when the
  // behavior is not running (will result in an error and no action). Also, if the behavior was running when
  // you called DelegateIfInControl, but is no longer running when the action completed, the callback will NOT be
  // called (this is necessary to prevent non-running behaviors from doing things with the robot). If the
  // behavior is stopped, within the Stop function, any actions which are still running will be canceled
  // (and you will not get any callback for it).
  //
  // Each DelegateIfInControl function returns true if the action was started, false otherwise. Reasons actions
  // might not be started include:
  // 1. Another action (from DelegateIfInControl) is currently running or queued
  // 2. You are not running ( IsActivated() returns false )
  //
  // DelegateIfInControl takes ownership of the action.  If the action is not
  // successfully queued, the action is immediately destroyed.
  //
  // Start an action now, and optionally provide a callback which will be called with the
  // RobotCompletedAction that corresponds to the action
  using RobotCompletedActionCallback =  BehaviorRobotCompletedActionCallback;
  bool DelegateIfInControl(IActionRunner* action, RobotCompletedActionCallback callback = {});

  bool DelegateIfInControl(IActionRunner* action, BehaviorRobotCompletedActionWithExternalInterfaceCallback callback);

  // helper that just looks at the result (simpler, but you can't get things like the completion union)
  using ActionResultCallback = BehaviorActionResultCallback;
  bool DelegateIfInControl(IActionRunner* action, ActionResultCallback callback);
  
  // helper that passes through both the ActionResult and Robot so that behaviors don't have
  // to store references to robot as much
  using ActionResultWithRobotCallback = BehaviorActionResultWithExternalInterfaceCallback;
  bool DelegateIfInControl(IActionRunner* action, ActionResultWithRobotCallback callback);

  // If you want to do something when the action finishes, regardless of its result, you can use the
  // following callbacks, with either a callback function taking no arguments or taking a single BehaviorExternalInterface&
  // argument. You can also use member functions. The callback will be called when action completes for any
  // reason (as long as the behavior is running)

  using SimpleCallback = BehaviorSimpleCallback;
  bool DelegateIfInControl(IActionRunner* action, SimpleCallback callback);

  using SimpleCallbackWithRobot = BehaviorSimpleCallbackWithExternalInterface;
  bool DelegateIfInControl(IActionRunner* action, SimpleCallbackWithRobot callback);

  template<typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(BehaviorExternalInterface& behaviorExternalInterface));

  template<typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(void));
  
  template<typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(ActionResult, BehaviorExternalInterface& behaviorExternalInterface));
  
  
  // If possible (without canceling anything), delegate to the given behavior and return true. Otherwise,
  // return false
  bool DelegateIfInControl(BehaviorExternalInterface& behaviorExternalInterface, IBehavior* delegate);
  
  // If possible (even if it means canceling delegated, delegate to the given behavior and return
  // true. Otherwise, return false (e.g. if the passed in interface doesn't have access to the delegation
  // component)
  bool DelegateNow(BehaviorExternalInterface& behaviorExternalInterface, IBehavior* delegate);
  
  // This function cancels the action started by DelegateIfInControl (if there is one). Returns true if an action was
  // canceled, false otherwise. Note that if you are activated, this will trigger a callback for the
  // cancellation unless you set allowCallback to false. If the action was created by a helper, the helper
  // will be canceled as well, unless allowHelperToContinue is true
  bool CancelDelegates(bool allowCallback = true, bool allowHelperToContinue = false);
  
  // Behaviors should call this function when they reach their completion state
  // in order to log das events and notify activity strategies if they listen for the message
  void BehaviorObjectiveAchieved(BehaviorObjective objectiveAchieved, bool broadcastToGame = true) const;

  // Behaviors can call this to register a needs action with the needs system
  // If needActionId is not specified, the previously-initialized _needActionId will be used
  void NeedActionCompleted(NeedsActionId needsActionId = NeedsActionId::NoAction);

  /////////////
  /// "Smart" helpers - Behaviors can call these functions to set properties that
  /// need to be cleared when the behavior stops.  ICozmoBehavior will hold the reference
  /// and clear it appropriately.  Functions also exist to clear these properties
  /// before the behavior stops.
  ////////////////
  
  // Push an idle animation which will be removed when the behavior stops
  void SmartPushIdleAnimation(BehaviorExternalInterface& behaviorExternalInterface, AnimationTrigger animation);
  
  // Remove an idle animation before the beahvior stops
  void SmartRemoveIdleAnimation(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Allows the behavior to disable and enable reaction triggers without having to worry about re-enabling them
  // these triggers will be automatically re-enabled when the behavior stops
  void SmartDisableReactionsWithLock(const std::string& lockID, const TriggersArray& triggers);
  
  // If a behavior needs to re-enable a reaction trigger for later stages after being
  // disabled with SmartDisablesableReactionaryBehavior  this function will re-enable the behavior
  // and stop tracking it
  void SmartRemoveDisableReactionsLock(const std::string& lockID);
  
  // Avoid calling this function directly, use the SMART_DISABLE_REACTION_DEV_ONLY macro instead
  // Locks a single reaction trigger instead of a full TriggersArray
#if ANKI_DEV_CHEATS
  void SmartDisableReactionWithLock(const std::string& lockID, const ReactionTrigger& trigger);
#endif

  // For the duration of this behavior, or until SmartClearMotionProfile() is called (whichever is sooner),
  // use the specified motion profile for all motions. Note that this will result in an error if the behavior
  // tries to manually set a speed or acceleration on an action. This may be called automatically based on
  // behavior json data here at the ICozmoBehavior level
  void SmartSetMotionProfile(const PathMotionProfile& motionProfile);
  void SmartClearMotionProfile();
  
  // Allows the behavior to lock and unlock tracks without worrying about the possibility of the behavior
  // being interrupted and leaving the track locked
  bool SmartLockTracks(u8 animationTracks, const std::string& who, const std::string& debugName);
  bool SmartUnLockTracks(const std::string& who);

  // Allows the behavior to set a custom light pattern which will be automatically canceled if the behavior ends
  bool SmartSetCustomLightPattern(const ObjectID& objectID,
                                  const CubeAnimationTrigger& anim,
                                  const ObjectLights& modifier = {});
  bool SmartRemoveCustomLightPattern(const ObjectID& objectID,
                                     const std::vector<CubeAnimationTrigger>& anims);
  
  // Ensures that a handle is stopped if the behavior is stopped
  bool SmartDelegateToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                             HelperHandle handleToRun,
                             SimpleCallbackWithRobot successCallback = nullptr,
                             SimpleCallbackWithRobot failureCallback = nullptr);

  template<typename T>
  bool SmartDelegateToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                             HelperHandle handleToRun,
                             void(T::*successCallback)(BehaviorExternalInterface& behaviorExternalInterface));

  template<typename T>
  bool SmartDelegateToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                             HelperHandle handleToRun,
                             void(T::*successCallback)(BehaviorExternalInterface& behaviorExternalInterface),
                             void(T::*failureCallback)(BehaviorExternalInterface& behaviorExternalInterface));

  // Stop a helper delegated with SmartDelegateToHelper
  bool StopHelperWithoutCallback();
  
  // Convenience function for setting behavior state lights in the behavior manager
  void SetBehaviorStateLights(const std::vector<BehaviorStateLightInfo>& structToSet, bool persistOnReaction);
  

  virtual void UpdateTargetBlocksInternal(BehaviorExternalInterface& behaviorExternalInterface) const {};
  
  // Convenience Method for accessing the behavior helper factory
  BehaviorHelperFactory& GetBehaviorHelperFactory();
  
  // If a behavior requires that an AIInformationProcess is running for the behavior
  // to operate properly, the behavior should set this variable directly so that
  // it's checked in WantsToBeActivatedBase
  AIInformationAnalysis::EProcess _requiredProcess;
  
  bool ShouldStreamline() const { return (_alwaysStreamline || _sparksStreamline); }
  
private:
  
  NeedsActionId ExtractNeedsActionIDFromConfig(const Json::Value& config);

  Robot* _robot;
  BehaviorExternalInterface* _behaviorExternalInterface;
  float _lastRunTime_s;
  float _activatedTime_s;

  // only used if we aren't using the BSM
  u32 _lastActionTag = 0;
  IStateConceptStrategyPtr _stateConceptStrategy;
  
  // Returns true if the state of the world/robot is sufficient for this behavior to be executed
  bool WantsToBeActivatedBase(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  bool ReadFromJson(const Json::Value& config);
  
  void HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg);
  
  // ==================== Member Vars ====================
  
  // The ID and a convenience cast of the ID to a string
  const BehaviorID  _id;
  const std::string _idString;
  
  std::string _displayNameKey = "";
  std::string _debugStateName = "";
  BehaviorClass _behaviorClassID;
  NeedsActionId _needsActionID;
  ExecutableBehaviorType _executableType;
  
  // if an unlockId is set, the behavior won't be activatable unless the unlockId is unlocked in the progression component
  UnlockId _requiredUnlockId;

  // required severe needs expression to run this activity
  NeedId _requiredSevereNeed;
  
  // if _requiredRecentDriveOffCharger_sec is greater than 0, this behavior is only activatable if last time the robot got off the charger by
  // itself was less than this time ago. Eg, a value of 1 means if we got off the charger less than 1 second ago
  float _requiredRecentDriveOffCharger_sec;
  
  // if _requiredRecentSwitchToParent_sec is greater than 0, this behavior is only activatable if last time its parent behavior
  // chooser was activated happened less than this time ago. Eg: a value of 1 means 'if the parent got activated less
  // than 1 second ago'. This allows some behaviors to run only first time that their parent is activated (specially for activities)
  // TODO rsam: differentiate between (de)activation and interruption
  float _requiredRecentSwitchToParent_sec;
  
  int _startCount = 0;

  // for when delegation finishes - if invalid, no action
  RobotCompletedActionCallback _actionCallback;
  bool _stopRequestedAfterAction = false;
  
  bool _isActivated;
  // should only be used to allow DelegateIfInControl to start while a behavior is resuming
  bool _isResuming;
  
  // A set of the locks that a behavior has used to disable reactions
  // these will be automatically re-enabled on behavior stop
  std::set<std::string> _smartLockIDs;
  
  // track whether the behavior has set an idle
  bool _hasSetIdle;
  
  // An int that holds tracks disabled using SmartLockTrack
  std::map<std::string, u8> _lockingNameToTracksMap;

  bool _hasSetMotionProfile = false;
  
  
  // Handle for SmartDelegateToHelper
  WeakHelperHandle _currentHelperHandle;

  //A list of object IDs that have had a custom light pattern set
  std::vector<ObjectID> _customLightObjects;
  
  // Allows behaviors to skip certain steps when streamlined
  // Set if this behavior is a sparked behavior
  bool _sparksStreamline = false;
  
  // Whether or not the behavior is always be streamlined (set via json)
  bool _alwaysStreamline = false;
  

  
  ///////
  // Tracking subscribe tags for initialization
  ///////
  std::unordered_map<GameToEngineTag, std::function<void(const GameToEngineEvent&)>> _gameToEngineCallbackMap;
  std::unordered_map<EngineToGameTag, std::function<void(const EngineToGameEvent&)>> _engineToGameCallbackMap;
  std::set<RobotInterface::RobotToEngineTag> _robotToEngineTags;

  // Tracking wants to run configs for initialization
  Json::Value _wantsToRunConfig;
  
////////
//// Scored Behavior functions
///////
  
public:
  // Allow us to load scored JSON in seperately from the rest of parameters
  bool ReadFromScoredJson(const Json::Value& config, const bool fromScoredChooser = true);
  
  // Stops the behavior immediately but gives it a couple of tick window during which score
  // evaluation will not include its activated penalty.  This allows behaviors to
  // stop themselves in hopes of being re-selected with new fast-forwarding and
  // block settings without knocking its score down so something else is selected
  void StopWithoutImmediateRepetitionPenalty(BehaviorExternalInterface& behaviorExternalInterface);
  
  
  float EvaluateScore(BehaviorExternalInterface& behaviorExternalInterface) const;
  const MoodScorer& GetMoodScorer() const { return _moodScorer; }
  void ClearEmotionScorers()                         { _moodScorer.ClearEmotionScorers(); }
  void AddEmotionScorer(const EmotionScorer& scorer) { _moodScorer.AddEmotionScorer(scorer); }
  size_t GetEmotionScorerCount() const { return _moodScorer.GetEmotionScorerCount(); }
  const EmotionScorer& GetEmotionScorer(size_t index) const { return _moodScorer.GetEmotionScorer(index); }
  
  float EvaluateRepetitionPenalty() const;
  const Util::GraphEvaluator2d& GetRepetitionPenalty() const { return _repetitionPenalty; }
  
  float EvaluateActivatedPenalty() const;
  const Util::GraphEvaluator2d& GetActivatedPenalty() const { return _activatedPenalty; }

protected:
  
  virtual void ScoredActingStateChanged(bool isActing);
  
  // EvaluateScoreInternal is used to score each behavior for behavior selection - it uses mood scorer or
  // flat score depending on configuration. If the behavior is activated, it
  // uses the activated score to decide if it should stay active
  virtual float EvaluateActivatedScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const;
  virtual float EvaluateScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  // Additional DelegateIfInControl definitions relating to score
  
  // Called after DelegateIfInControl, will add extraScore to the result of EvaluateActivatedScoreInternal. This makes
  // it easy to encourage the system to keep a behavior activated while control is delegated. NOTE: multiple calls to
  // this function (for the same action) will be cumulative. The bonus will be reset as soon as the action is
  // complete, or the behavior is no longer active
  void IncreaseScoreWhileControlDelegated(float extraScore);
  
  // Convenience wrappers that combine IncreaseScoreWhileControlDelegated with DelegateIfInControl,
  // including any of the callback types above
  virtual bool DelegateIfInControlExtraScore(IActionRunner* action, float extraScoreWhileControlDelegated) final;
  
  template<typename CallbackType>
  bool DelegateIfInControlExtraScore(IActionRunner* action, float extraScoreWhileControlDelegated, CallbackType callback);
  
  bool WantsToBeActivatedScored() const;
  
private:
  void ScoredInit(BehaviorExternalInterface& behaviorExternalInterface);
  void ActivatedScored();
  
  void HandleBehaviorObjective(const ExternalInterface::BehaviorObjectiveAchieved& msg);
  
  // ==================== Member Vars ====================
  MoodScorer              _moodScorer;
  Util::GraphEvaluator2d  _repetitionPenalty;
  Util::GraphEvaluator2d  _activatedPenalty;
  float                   _flatScore = 0;
  float                   _extraActivatedScore = 0;
  #if ANKI_DEV_CHEATS
    // Used to verify that all scoring configs loaded in are equal
    Json::Value             _scoringValuesCached;
  #endif
  // used to allow short times during which repetitionPenalties don't apply
  float                   _nextTimeRepetitionPenaltyApplies_s = 0;
  
  // if this behavior objective gets sent (by any behavior), then consider this behavior to have been activated
  // (for purposes of repetition penalty, aka cooldown)
  BehaviorObjective _cooldownOnObjective = BehaviorObjective::Count;
  
  
  bool _enableRepetitionPenalty = true;
  bool _enableActivatedPenalty = true;
  
  // Keep track of the number of times resumed from
  int _timesResumedFromPossibleInfiniteLoop = 0;
  float _timeCanRunAfterPossibleInfiniteLoopCooldown_sec = 0;
  
}; // class ICozmoBehavior

  
  
template<typename T>
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, void(T::*callback)(BehaviorExternalInterface& behaviorExternalInterface))
{
  return DelegateIfInControl(action, std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}

template<typename T>
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, void(T::*callback)(void))
{
  std::function<void(void)> boundCallback = std::bind(callback, static_cast<T*>(this));
  return DelegateIfInControl(action, boundCallback);
}

template<typename T>
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, void(T::*callback)(ActionResult, BehaviorExternalInterface& behaviorExternalInterface))
{
  return DelegateIfInControl(action, std::bind(callback, static_cast<T*>(this), std::placeholders::_1, std::placeholders::_2));
}



template<typename T>
bool ICozmoBehavior::SmartDelegateToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                      HelperHandle handleToRun,
                                      void(T::*successCallback)(BehaviorExternalInterface& behaviorExternalInterface))
{
  return SmartDelegateToHelper(behaviorExternalInterface, handleToRun,
                               std::bind(successCallback, static_cast<T*>(this), std::placeholders::_1));
}

template<typename T>
bool ICozmoBehavior::SmartDelegateToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                      HelperHandle handleToRun,
                                      void(T::*successCallback)(BehaviorExternalInterface& behaviorExternalInterface),
                                      void(T::*failureCallback)(BehaviorExternalInterface& behaviorExternalInterface))
{
  return SmartDelegateToHelper(behaviorExternalInterface, handleToRun,
                               std::bind(successCallback, static_cast<T*>(this), std::placeholders::_1),
                               std::bind(failureCallback, static_cast<T*>(this), std::placeholders::_1));
}


////////
//// Scored Behavior functions
///////
  
inline bool ICozmoBehavior::DelegateIfInControlExtraScore(IActionRunner* action, float extraScoreWhileControlDelegated) {
  IncreaseScoreWhileControlDelegated(extraScoreWhileControlDelegated);
  return DelegateIfInControl(action);
}

template<typename CallbackType>
inline bool ICozmoBehavior::DelegateIfInControlExtraScore(IActionRunner* action, float extraScoreWhileControlDelegated, CallbackType callback) {
  IncreaseScoreWhileControlDelegated(extraScoreWhileControlDelegated);
  return DelegateIfInControl(action, callback);
}

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_ICozmoBehavior_H__
