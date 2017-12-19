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
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/robotInterface/messageHandler.h"
#include <set>


#include "clad/types/actionResults.h"
#include "clad/types/behaviorComponent/behaviorObjectives.h"
#include "clad/types/needsSystemTypes.h"
#include "clad/types/needsSystemTypes.h"
#include "clad/types/unlockTypes.h"
#include "util/logging/logging.h"

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
class DriveToObjectAction;
class BehaviorHelperFactory;
class IHelper;
enum class ObjectInteractionIntention;
enum class CloudIntent : uint8_t;

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
  static void InjectBehaviorClassAndIDIntoConfig(BehaviorClass behaviorClass, BehaviorID behaviorID, Json::Value& config);  
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
  void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override final;
    
  // This behavior was active, but is now stopping (to make way for a new current
  // behavior). Any behaviors from DelegateIfInControl will be canceled.
  void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) final override;

  // optional override for child classes to be called when they are deactivated
  virtual void OnCozmoBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) {}

  
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

  // default is no delegates, but behaviors which delegate can overload this
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override { }
  

  inline void SetDebugStateName(const std::string& inName) {
    PRINT_CH_INFO("Behaviors", "Behavior.TransitionToState", "Behavior:%s, FromState:%s ToState:%s",
                  GetIDStr().c_str(), _debugStateName.c_str(), inName.c_str());
    _debugStateName = inName;
  }
  
  
  virtual void OnEnteredActivatableScopeInternal() override;
  virtual void OnLeftActivatableScopeInternal() override;

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  
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

  // same as above but with a callback that will get called as soon as the delegate stops itself (regardless
  // of why)
  bool DelegateIfInControl(BehaviorExternalInterface& behaviorExternalInterface,
                           IBehavior* delegate,
                           BehaviorSimpleCallbackWithExternalInterface callback);

  template<typename T>
  bool DelegateIfInControl(BehaviorExternalInterface& behaviorExternalInterface,
                           IBehavior* delegate,
                           void(T::*callback)(BehaviorExternalInterface& behaviorExternalInterface));

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
  // cancellation unless you set allowCallback to false. If the action was created by a helper, the helper
  // will be canceled as well, unless allowHelperToContinue is true
  bool CancelDelegates(bool allowCallback = true, bool allowHelperToContinue = false);

  // This function cancels this behavior. It never allows callbacks or helpers to continue. This is just a
  // convenience wrapper for calling CancelSelf on the delegation component. It returns true if the behavior
  // was successfully canceled, false otherwise (e.g. the behavior wasn't active to begin with)
  bool CancelSelf();
  
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

  virtual void UpdateTargetBlocksInternal(BehaviorExternalInterface& behaviorExternalInterface) const {};
  
  // Convenience Method for accessing the behavior helper factory
  BehaviorHelperFactory& GetBehaviorHelperFactory();
  
  // If a behavior requires that an AIInformationProcess is running for the behavior
  // to operate properly, the behavior should set this variable directly so that
  // it's checked in WantsToBeActivatedBase
  AIInformationAnalysis::EProcess _requiredProcess;
  
  bool ShouldStreamline() const { return (_alwaysStreamline); }
  
private:
  
  NeedsActionId ExtractNeedsActionIDFromConfig(const Json::Value& config);

  BehaviorExternalInterface* _behaviorExternalInterface;
  float _lastRunTime_s;
  float _activatedTime_s;

  // only used if we aren't using the BSM
  u32 _lastActionTag = 0;
  std::vector<IBEIConditionPtr> _wantsToBeActivatedStrategies;
  
  // Returns true if the state of the world/robot is sufficient for this behavior to be executed
  bool WantsToBeActivatedBase(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  bool ReadFromJson(const Json::Value& config);
  
  void HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg);

  // hide behaviorTypes.h file in .cpp
  std::string GetClassString(BehaviorClass behaviorClass) const;
  
  // ==================== Member Vars ====================
  
  // The ID and a convenience cast of the ID to a string
  const BehaviorID  _id;
  const std::string _idString;
  
  std::string _displayNameKey = "";
  std::string _debugStateName = "";
  BehaviorClass _behaviorClassID;
  NeedsActionId _needsActionID;
  ExecutableBehaviorType _executableType;
  int _timesResumedFromPossibleInfiniteLoop = 0;
  float _timeCanRunAfterPossibleInfiniteLoopCooldown_sec = 0.f;
  
  // when respond to cloud intent is set the behavior will
  // 1) WantToBeActivated when that intent is pending
  // 2) Clear the intent when the behavior is activated
  CloudIntent _respondToCloudIntent;

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

  // for when delegation to a _behavior_ finishes. If invalid, no callback
  BehaviorSimpleCallbackWithExternalInterface _behaviorDelegateCallback;
  
  bool _isActivated;
  
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
  
  // Whether or not the behavior is always be streamlined (set via json)
  bool _alwaysStreamline = false;
  

  
  ///////
  // Tracking subscribe tags for initialization
  ///////
  std::unordered_map<GameToEngineTag, std::function<void(const GameToEngineEvent&)>> _gameToEngineCallbackMap;
  std::unordered_map<EngineToGameTag, std::function<void(const EngineToGameEvent&)>> _engineToGameCallbackMap;
  std::set<RobotInterface::RobotToEngineTag> _robotToEngineTags;

  // Behaviors can load in internal "anonymous" behaviors which are not stored
  // in the behavior container and are referenced by string instead of by ID
  // This map provides built in "anonymous" functionality, but behaviors
  // can also load anonymous behaviors directly into variables using the
  // anonymous behavior factory
  Json::Value _anonymousBehaviorMapConfig;  
  std::map<std::string,ICozmoBehaviorPtr> _anonymousBehaviorMap;
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


template<typename T>
bool ICozmoBehavior::DelegateIfInControl(BehaviorExternalInterface& behaviorExternalInterface,
                                         IBehavior* delegate,
                                         void(T::*callback)(BehaviorExternalInterface& behaviorExternalInterface))
{
  return DelegateIfInControl(behaviorExternalInterface,
                             delegate,
                             std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}

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

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_ICozmoBehavior_H__
