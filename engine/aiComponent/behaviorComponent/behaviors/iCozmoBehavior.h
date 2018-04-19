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
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/components/cubes/cubeLightComponent.h"
#include "engine/components/visionScheduleMediator/iVisionModeSubscriber.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator_fwd.h"
#include "engine/robotInterface/messageHandler.h"
#include <set>


#include "clad/types/actionResults.h"
#include "clad/types/behaviorComponent/behaviorObjectives.h"
#include "clad/types/unlockTypes.h"
#include "util/console/consoleVariable.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

//Transforms enum into string
#define DEBUG_SET_STATE(s) SetDebugStateName(#s)

namespace Anki {
namespace Util{
class RandomGenerator;
}
class ObjectID;
namespace Cozmo {
  
// Forward declarations
class ActionableObject;
class ConditionUserIntentPending;
class DriveToObjectAction;
enum class ObjectInteractionIntention;
class UnitTestKey;
class UserIntent;
enum class UserIntentTag : uint8_t;

class ISubtaskListener;
class IReactToFaceListener;
class IReactToObjectListener;
class IReactToPetListener;
class IFistBumpListener;
class IFeedingListener;

enum class CubeAnimationTrigger;

struct PathMotionProfile;

namespace ExternalInterface {
struct BehaviorObjectiveAchieved;
}


// This struct defines some of the operation modes iCozmoBehavior
// provides to derived classes. They have the opportunity to override
// the default values set below in order to change the way the behavior
// operates
struct BehaviorOperationModifiers{
  BehaviorOperationModifiers(){
    visionModesForActivatableScope = std::make_unique<std::set<VisionModeRequest>>();
    visionModesForActiveScope = std::make_unique<std::set<VisionModeRequest>>();
  }

  // WantsToBeActivated modifiers
  bool wantsToBeActivatedWhenCarryingObject = false;
  bool wantsToBeActivatedWhenOffTreads = false;
  bool wantsToBeActivatedWhenOnCharger = true;

  // If true iCozmoBehavior will automatically cancel the behavior if it hasn't delegated control by the end of its tick
  // Default is True for two reasons:
  //   1) Most behaviors don't want the robot to sit still not doing anything
  //      so they are always delegating or have hit an unexpected bug, and it's better to cancel so the robot doesn't freeze
  //   2) It supports open-ended final Delegation calls. After the final delegation the behavior doesn't have to
  //      monitor its status - it will be automatically canceled when the final delegation completes.
  // Override to false if the behavior will always cancel itself when it's done
  bool behaviorAlwaysDelegates = true;

  // Behaviors which require vision processing can add requests to these vectors to have the base class
  // manage subscriptions to those VisionModes. Default is none.
  std::unique_ptr<std::set<VisionModeRequest>> visionModesForActivatableScope;
  std::unique_ptr<std::set<VisionModeRequest>> visionModesForActiveScope;
};

// Base Behavior Interface specification
class ICozmoBehavior : public IBehavior, public IVisionModeSubscriber
{
protected:  
  friend class BehaviorFactory;

  // Can't create a public ICozmoBehavior, but derived classes must pass a robot
  // reference into this protected constructor.
  ICozmoBehavior(const Json::Value& config);
    
  virtual ~ICozmoBehavior();
    
public:  
  static Json::Value CreateDefaultBehaviorConfig(BehaviorClass behaviorClass, BehaviorID behaviorID);
  static void InjectBehaviorClassAndIDIntoConfig(BehaviorClass behaviorClass, BehaviorID behaviorID, Json::Value& config);  
  static BehaviorID ExtractBehaviorIDFromConfig(const Json::Value& config, const std::string& fileName = "");
  static BehaviorClass ExtractBehaviorClassFromConfig(const Json::Value& config);

  // After Init is called, this function should be called on every behavior to set the operation modifiers. It
  // will internally call GetBehaviorOperationModifiers() as needed
  void InitBehaviorOperationModifiers();

  bool IsActivated() const { return _isActivated; }

  // returns true if the behavior has delegated control to a action/behavior
  bool IsControlDelegated() const;

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
  void OnActivatedInternal() override final;
    
  // This behavior was active, but is now stopping (to make way for a new current
  // behavior). Any behaviors from DelegateIfInControl will be canceled.
  void OnDeactivatedInternal() final override;
  
  // Returns true if the state of the world/robot is sufficient for this behavior to be executed
  bool WantsToBeActivatedInternal() const override final;

  BehaviorID         GetID()      const { return _id; }

  const std::string& GetDebugStateName() const { return _debugStateName;}
  ExecutableBehaviorType GetExecutableType() const { return _executableType; }
  const BehaviorClass GetClass() const { return _behaviorClassID; }


  // Return true if this is a good time to interrupt this behavior. This allows more gentle interruptions for
  // things which aren't immediately urgent. Eventually this may become a mandatory override, but for now, the
  // default is that we can never interrupt gently
  virtual bool CanBeGentlyInterruptedNow() const { return false; }

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

  // Give derived behaviors the opportunity to override default behavior operations
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const = 0;
  
  // gets list of ICozmoBehavior's expected keys and the derived class's, through GetBehaviorJsonKeys
  std::vector<const char*> GetAllJsonKeys() const;

  // Allow external behaviors to ask that this behavior return false for WantsToBeActivated
  // for this tick. This should only be used by "coordinator" behaviors to allow an "event"
  // to bypass one of the behaviors InActivatableScope so that it can be handled by something
  // further down the stack
  void SetDontActivateThisTick(const std::string& coordinatorName);
  
  std::map<std::string,ICozmoBehaviorPtr> TESTONLY_GetAnonBehaviors( UnitTestKey key ) const;

protected:


  // default is no delegates, but behaviors which delegate can overload this
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override { }

  inline void SetDebugStateName(const std::string& inName) {
    PRINT_CH_INFO("Behaviors", "Behavior.TransitionToState", "Behavior:%s, FromState:%s ToState:%s",
                  GetDebugLabel().c_str(), _debugStateName.c_str(), inName.c_str());
    _debugStateName = inName;
  }
  
  using EvalUserIntentFunc = std::function<bool(const UserIntent&)>; // should be true if data matches
  
  // You can add any number of user intents that you wish to wait for. If there is a pending intent
  // and it matches, based on any of the conditions you supply, then this ICozmoBehavior will
  // WantsToBeActivated(), in the absence of any other blocking conditions. When the behavior
  // activates, it will clear the pending intent that matched one of your conditions.
  // The functionality of all of these methods is also available in your behavior instance json
  // with the parameter "respondToUserIntents," but sometimes it's more appropriate to use code.
  // All of these methods must be called before Init() is called. You may:
  // Wait for a match with the specfic tag
  void AddWaitForUserIntent( UserIntentTag intentTag );
  // Wait for an intent, which must match in entirety.
  void AddWaitForUserIntent( UserIntent&& intent );
  // Wait for a match with tag AND func(intent). The func will only be evaluated if the tag matches,
  // so your lambda only needs to worry about the params, not the tag.
  void AddWaitForUserIntent( UserIntentTag tag, EvalUserIntentFunc&& func );
  
  // remove all of the conditions added above
  void ClearWaitForUserIntent();
  
  
  // Set whether this behavior responds to the trigger word. It's only valid to call this before Init
  // is called (i.e. from the constructor). Normally this can be specified in json, but in some cases
  // it may be more desirable to set it from code
  void SetRespondToTriggerWord(bool shouldRespond);
  
  // Add to a list of BehaviorTimerManager timers that this behavior should reset upon activation.
  // You don't want to put a reset in too high (/ deep in the stack) a behavior in case one
  // of its delegates fails, which might reset the timer without the action being performed. You
  // also don't want to put it in too low a delegate for the same reason -- if it doesn't start,
  // the higher level behavior might try again, and you end up in a loop. Choose wisely my friend.
  void AddResetTimer(const std::string& timerName) { _resetTimers.push_back(timerName); }
  
  // Derived behaviors can specify all json keys they expect at the root level, so that
  // behavior json loading fails (in dev) if unexpected keys are found. You should insert
  // into this list instead of assigning it.
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const = 0;
  
  virtual void OnEnteredActivatableScopeInternal() override;
  virtual void OnLeftActivatableScopeInternal() override;

  virtual void OnBehaviorEnteredActivatableScope() { }
  virtual void OnBehaviorLeftActivatableScope() { }

  virtual void OnBehaviorActivated() = 0;
  
  void InitInternal() override final;
  virtual void InitBehavior() {};
  
  // To keep passing through data generic, if robot is not overridden
  // check the NoPreReqs WantsToBeActivatedBehavior
  virtual bool WantsToBeActivatedBehavior() const = 0;

  // This function can be implemented by behaviors. It should return Running while it is running, and Complete
  // or Failure as needed. If it returns Complete, Stop will be called. Default implementation is to
  // return Running while ControlIsDelegated, and Complete otherwise
  virtual void UpdateInternal() override final;
  virtual void BehaviorUpdate() {};
  virtual void OnBehaviorDeactivated() { };

  Util::RandomGenerator& GetRNG() const;
  
  void SubscribeToTags(std::set<GameToEngineTag>&& tags);
  void SubscribeToTags(std::set<EngineToGameTag>&& tags);
  void SubscribeToTags(std::set<RobotInterface::RobotToEngineTag>&& tags);
  
  // Function that calls message handling helper functions
  void UpdateMessageHandlingHelpers();

  // Derived classes must override this method to handle events that come in
  // irrespective of whether the behavior is running or not. Note that the Robot
  // reference is const to prevent the behavior from modifying the robot when it
  // is not running. If the behavior is subscribed to multiple tags, the presumption
  // is that this will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called before HandleWhileRunning and HandleWhileNotRunning!
  virtual void AlwaysHandleInScope(const GameToEngineEvent& event) { }
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) { }
  virtual void AlwaysHandleInScope(const RobotToEngineEvent& event) { }
  
  // Derived classes must override this method to handle events that come in
  // while the behavior is running. In this case, the behavior is allowed to
  // modify the robot and thus receives a non-const reference to it.
  // If the behavior is subscribed to multiple tags, the presumption is that it
  // will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called first!
  virtual void HandleWhileActivated(const GameToEngineEvent& event) { }
  virtual void HandleWhileActivated(const EngineToGameEvent& event) { }
  virtual void HandleWhileActivated(const RobotToEngineEvent& event) { }
  
  // Derived classes must override this method to handle events that come in
  // only while the behavior is NOT running. If it doesn't matter whether the
  // behavior is running or not, consider using AlwaysHandle above instead.
  // If the behavior is subscribed to multiple tags, the presumption is that it
  // will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called first!
  virtual void HandleWhileInScopeButNotActivated(const GameToEngineEvent& event) { }
  virtual void HandleWhileInScopeButNotActivated(const EngineToGameEvent& event) { }
  virtual void HandleWhileInScopeButNotActivated(const RobotToEngineEvent& event) { }
  
  // Many behaviors use a pattern of delegating control to a behavior or action, then waiting for it to finish 
  // before selecting what to do next. Behaviors can use the functions below to pass lambdas or member functions
  // as callbacks when delegation finishes instead of monitoring for changes in IsControlDelegated() directly.
  // Standard delegation restrictions apply: 
  //   1) You must active and on top of the behavior stack to delegate
  //   2) You may only delegate to one action or behavior at a time
  // Callbacks are cleared when the behavior leaves activated scope
  //
  // DelegateIfInControl takes ownership of actions.  If the action is not
  // successfully queued, the action is immediately destroyed.
  //

  using SimpleCallback = BehaviorSimpleCallback;
  using RobotCompletedActionCallback =  BehaviorRobotCompletedActionCallback;
  using ActionResultCallback = BehaviorActionResultCallback;

  // DelegateNow functions will automatically cancel delegates and then delegate
  // Delegation can still fail if the behavior is not in charge of the behavior stack
  bool DelegateNow(IActionRunner* action, RobotCompletedActionCallback callback = {});
  bool DelegateNow(IActionRunner* action, ActionResultCallback callback);
  bool DelegateNow(IActionRunner* action, SimpleCallback callback);

  bool DelegateNow(IBehavior* delegate, SimpleCallback callback = {});

  // DelegateIfInControl functions will only successfully delegate if the behavior
  // is currently in control of the behavior stack and has not delegated to an action
  bool DelegateIfInControl(IActionRunner* action, RobotCompletedActionCallback callback = {});
  bool DelegateIfInControl(IActionRunner* action, ActionResultCallback callback);
  bool DelegateIfInControl(IActionRunner* action, SimpleCallback callback);

  bool DelegateIfInControl(IBehavior* delegate, SimpleCallback callback = {});

  // DelegateNow templated functions - allow member functions to be used for callbacks
  template<typename T>
  bool DelegateNow(IActionRunner* action, void(T::*callback)(RobotCompletedActionCallback));
  template<typename T>
  bool DelegateNow(IActionRunner* action, void(T::*callback)(ActionResult));
  template<typename T>
  bool DelegateNow(IActionRunner* action, void(T::*callback)());

  template<typename T>
  bool DelegateNow(IBehavior* delegate, void(T::*callback)());


  // DelegateIfInControl templated functions - allow member functions to be used for callbacks
  template<typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(RobotCompletedActionCallback));
  template<typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(ActionResult));
  template<typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)());

  template<typename T>
  bool DelegateIfInControl(IBehavior* delegate, void(T::*callback)());



  // Behaviors can easily create delegates using "anonymous" behaviors in their config file (see _anonymousBehaviorMap
  // comments below).  This function enables access to those anonymous behaviors.
  ICozmoBehaviorPtr FindAnonymousBehaviorByName(const std::string& behaviorName) const;

  // Sometimes it's necessary to downcast to a behavior to a specific behavior pointer, e.g. so an Activity
  // can access it's member functions. This function will help with that and provide a few assert checks along
  // the way. It sets outPtr in arguemnts, and returns true if the cast is successful
  template<typename T>
  bool FindAnonymousBehaviorByNameAndDowncast(const std::string& behaviorName,
                                                BehaviorClass requiredClass,
                                                std::shared_ptr<T>& outPtr ) const;

  // This function cancels the action started by DelegateIfInControl (if there is one). Returns true if an action was
  // canceled, false otherwise. Note that if you are activated, this will trigger a callback for the
  // cancellation unless you set allowCallback to false.
  bool CancelDelegates(bool allowCallback = true);

  // This function cancels this behavior. It never allows callbacks to continue. This is just a
  // convenience wrapper for calling CancelSelf on the delegation component. It returns true if the behavior
  // was successfully canceled, false otherwise (e.g. the behavior wasn't active to begin with)
  bool CancelSelf();
  
  // Behaviors should call this function when they reach their completion state
  // in order to log das events and notify activity strategies if they listen for the message
  void BehaviorObjectiveAchieved(BehaviorObjective objectiveAchieved, bool broadcastToGame = true) const;

  /////////////
  /// "Smart" helpers - Behaviors can call these functions to set properties that
  /// need to be cleared when the behavior stops.  ICozmoBehavior will hold the reference
  /// and clear it appropriately.  Functions also exist to clear these properties
  /// before the behavior stops.
  ////////////////

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

  // Helper function to play an emergency get out through the continuity component
  void PlayEmergencyGetOut(AnimationTrigger anim);

  template<typename T>
  T& GetAIComp() const {
    return GetBEI().GetAIComponent(). template GetComponent<T>();
  }

  template<typename T>
  T& GetBehaviorComp() const {
    return GetBEI().GetAIComponent().GetComponent<BehaviorComponent>(). template GetComponent<T>();
  }
  
  // If a behavior requires that an AIInformationProcess is running for the behavior
  // to operate properly, the behavior should set this variable directly so that
  // it's checked in WantsToBeActivatedBase
  AIInformationAnalysis::EProcess _requiredProcess;
  
  bool ShouldStreamline() const { return (_alwaysStreamline); }
  
  // If you _know_ you have been activated due to a pending intent, get the intent along with
  // its data using this helper
  UserIntent& GetTriggeringUserIntent();
  
  // make a member variable a console var that is only around as long as its class instance is
  #if ANKI_DEV_CHEATS
    template <typename T>
    void MakeMemberTunable(T& param, const std::string& name, const char* category = nullptr);
  #else // no op
    template <typename T>
    void MakeMemberTunable(T& param, const std::string& name, const char* category = nullptr) {  }
  #endif
  
private:
  
  std::string MakeUniqueDebugLabelFromConfig(const Json::Value& config);

  float _lastRunTime_s;
  float _activatedTime_s;

  // only used if we aren't using the BSM
  u32 _lastActionTag = 0;
  std::vector<IBEIConditionPtr> _wantsToBeActivatedConditions;
  std::vector<IBEIConditionPtr> _wantsToCancelSelfConditions;
  BehaviorOperationModifiers _operationModifiers;
  
  // Returns true if the state of the world/robot is sufficient for this behavior to be executed
  bool WantsToBeActivatedBase() const;
  
  bool ReadFromJson(const Json::Value& config);
  
  // Checks to see if delegation has completed and callbacks should run
  void CheckDelegationCallbacks();

  // hide behaviorTypes.h file in .cpp
  std::string GetClassString(BehaviorClass behaviorClass) const;
  
  // ==================== Member Vars ====================
  
  // The ID and a convenience cast of the ID to a string
  const BehaviorID  _id;
  
  std::string _debugStateName = "";
  BehaviorClass _behaviorClassID;
  ExecutableBehaviorType _executableType;
  int _timesResumedFromPossibleInfiniteLoop = 0;
  float _timeCanRunAfterPossibleInfiniteLoopCooldown_sec = 0.f;

  // when initialized with a condition, this ICozmoBehavior will
  // 1) Return false from WantToBeActivated if the condition evaluates to false (and true in
  //    the absence of other negative conditions), and
  // 2) Clear the intent when the behavior is activated
  std::shared_ptr< ConditionUserIntentPending > _respondToUserIntent;
  
  // if config changes this to false, the intent will be cleared but not moved into _pendingIntent,
  // below. It will instead remain in the UserIntentComponent as a backup
  bool _claimUserIntentData;
  
  // if a behavior is waiting for a specific intent and it got it, and _claimUserIntentData,
  // then the intent will be here just prior to activation. otherwise it will be null
  std::unique_ptr<UserIntent> _pendingIntent;
  
  // true when the trigger word is pending, in which case ICozmoBehavior will
  // 1) WantToBeActivated, in the absence of other negative conditions
  // 2) Clear the trigger word when the behavior is activated
  bool _respondToTriggerWord;
  
  // a list of named timers in the BehaviorTimerManager that should be reset when this behavior starts
  std::vector<std::string> _resetTimers;

  // Parameters that track SetDontActivateThisTick
  std::string _dontActivateCoordinator;
  size_t _tickDontActivateSetFor = -1;

  // If non-empty, trigger this emotion event when this behavior activated
  std::string _emotionEventOnActivated;

  // if an unlockId is set, the behavior won't be activatable unless the unlockId is unlocked in the progression component
  UnlockId _requiredUnlockId;
  
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
  RobotCompletedActionCallback _delegationCallback;
  
  bool _isActivated;
  
  // A set of the locks that a behavior has used to disable reactions
  // these will be automatically re-enabled on behavior stop
  std::set<std::string> _smartLockIDs;
  
  // An int that holds tracks disabled using SmartLockTrack
  std::map<std::string, u8> _lockingNameToTracksMap;

  bool _hasSetMotionProfile = false;

  //A list of object IDs that have had a custom light pattern set
  std::vector<ObjectID> _customLightObjects;
  
  // Whether or not the behavior is always be streamlined (set via json)
  bool _alwaysStreamline = false;
  
  bool _initHasBeenCalled = false;
  
  
  ///////
  // Tracking subscribe tags for initialization
  ///////
  std::set<GameToEngineTag> _gameToEngineTags;
  std::set<EngineToGameTag> _engineToGameTags;
  std::set<RobotInterface::RobotToEngineTag> _robotToEngineTags;

  // Behaviors can load in internal "anonymous" behaviors which are not stored
  // in the behavior container and are referenced by string instead of by ID
  // This map provides built in "anonymous" functionality, but behaviors
  // can also load anonymous behaviors directly into variables using the
  // anonymous behavior factory
  Json::Value _anonymousBehaviorMapConfig;  
  std::map<std::string,ICozmoBehaviorPtr> _anonymousBehaviorMap;
  
  // list of member vars in this behavior that have been marked as tunable. upon class
  // desctruction, each console var will be unregistered.
  std::vector< std::unique_ptr<Anki::Util::IConsoleVariable> > _tunableParams;
  
}; // class ICozmoBehavior


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::DelegateNow(IActionRunner* action, void(T::*callback)(RobotCompletedActionCallback))
{
  return DelegateNow(action, std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::DelegateNow(IActionRunner* action, void(T::*callback)(ActionResult))
{
  return DelegateNow(action, std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::DelegateNow(IActionRunner* action, void(T::*callback)(void))
{
  SimpleCallback unambiguous = std::bind(callback, static_cast<T*>(this));
  return DelegateNow(action, unambiguous);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::DelegateNow(IBehavior* delegate, void(T::*callback)())
{
  return DelegateNow(delegate,
                     std::bind(callback, static_cast<T*>(this)));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, void(T::*callback)(RobotCompletedActionCallback))
{
  return DelegateIfInControl(action, std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, void(T::*callback)(ActionResult))
{
  return DelegateIfInControl(action, std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::DelegateIfInControl(IActionRunner* action, void(T::*callback)(void))
{
  SimpleCallback unambiguous = std::bind(callback, static_cast<T*>(this));
  return DelegateIfInControl(action, unambiguous);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::DelegateIfInControl(IBehavior* delegate, void(T::*callback)())
{
  return DelegateIfInControl(delegate,
                             std::bind(callback, static_cast<T*>(this)));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
bool ICozmoBehavior::FindAnonymousBehaviorByNameAndDowncast(const std::string& behaviorName,
                                                              BehaviorClass requiredClass,
                                                              std::shared_ptr<T>& outPtr) const
{
  ICozmoBehaviorPtr behavior = FindAnonymousBehaviorByName(behaviorName);
  if( ANKI_VERIFY(behavior != nullptr,
                  "ICozmoBehavior.FindAnonymousBehaviorByNameAndDowncast.NoBehavior",
                  "BehaviorName: %s requiredClass: %s",
                  behaviorName.c_str(),
                  GetClassString(requiredClass).c_str()) &&
     
     ANKI_VERIFY(behavior->GetClass() == requiredClass,
                 "ICozmoBehavior.FindAnonymousBehaviorByNameAndDowncast.WrongClass",
                 "BehaviorName: %s requiredClass: %s",
                 behaviorName.c_str(),
                 GetClassString(requiredClass).c_str()) ) {
       
       outPtr = std::static_pointer_cast<T>(behavior);
       
       if( ANKI_VERIFY(outPtr != nullptr, "ICozmoBehavior.FindAnonymousBehaviorByNameAndDowncast.CastFailed",
                       "BehaviorName: %s requiredClass: %s",
                       behaviorName.c_str(),
                       GetClassString(requiredClass).c_str()) ) {
         return true;
       }
     }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if ANKI_DEV_CHEATS
template <typename T>
void ICozmoBehavior::MakeMemberTunable(T& param, const std::string& name, const char* category)
{
  std::string uniqueName = GetDebugLabel() + "_" + name;
  Anki::Util::StringReplace( uniqueName, "@", "" );
  static const bool unregisterOnDestruction = true;
  if( category == nullptr ) {
    category = "BehaviorInstanceParams";
  }
  // ensure this param isnt already registered
  for( const auto& var : _tunableParams ) {
    if( !ANKI_VERIFY( var->GetID() != uniqueName,
                      "ICozmoBehavior.MakeMemberTunable.AlreadyExists",
                      "Per-instance console var '%s' already exists",
                      uniqueName.c_str() ) )
    {
      return;
    }
  }
  _tunableParams.emplace_back( new Util::ConsoleVar<T>( param, uniqueName.c_str(), category, unregisterOnDestruction ) );
}
#endif

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_ICozmoBehavior_H__
