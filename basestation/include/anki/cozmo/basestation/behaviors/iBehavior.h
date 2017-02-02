/**
 * File: iBehavior.h
 *
 * Author: Andrew Stein : Kevin M. Karol
 * Date:   7/30/15  : 12/1/16
 *
 * Description: Defines interface for a Cozmo "Behavior"
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehavior_H__
#define __Cozmo_Basestation_Behaviors_IBehavior_H__

#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/aiInformationAnalysis/aiInformationAnalysisProcessTypes.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupFlags.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqNone.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/moodSystem/moodScorer.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include <set>

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "clad/types/actionResults.h"
#include "clad/types/behaviorObjectives.h"
#include "clad/types/behaviorTypes.h"
#include "clad/types/unlockTypes.h"
#include "util/logging/logging.h"
#include "util/signals/simpleSignal_fwd.h"

//Transforms enum into string
#define DEBUG_SET_STATE(s) SetDebugStateName(#s)

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
  
class BehaviorPreReqNone;
class BehaviorPreReqRobot;
class BehaviorPreReqAcknowledgeObject;
class BehaviorPreReqAcknowledgeFace;
class BehaviorPreReqAcknowledgePet;
  
class ISubtaskListener;
class IReactToFaceListener;
class IReactToObjectListener;
class IReactToPetListener;

enum class CubeAnimationTrigger;

namespace ExternalInterface {
class MessageEngineToGame;
class MessageGameToEngine;
struct RobotCompletedAction;
struct BehaviorObjectiveAchieved;
}
template<typename TYPE> class AnkiEvent;
  
// Base Behavior Interface specification
class IBehavior
{
protected:  
  friend class BehaviorFactory;

  // Can't create a public IBehavior, but derived classes must pass a robot
  // reference into this protected constructor.
  IBehavior(Robot& robot, const Json::Value& config);
    
  virtual ~IBehavior();
    
public:

  enum class Status {
    Failure,
    Running,
    Complete
  };
    
  bool IsRunning() const { return _isRunning; }
  // returns true if any action from StartAction is currently running, indicating that the behavior is
  // likely waiting for something to complete
  inline bool IsActing() const;

  // returns the number of times this behavior has been started (number of times Init was called and returned
  // OK, not counting calls to Resume)
  int GetNumTimesBehaviorStarted() const { return _startCount; }
  void ResetStartCount() { _startCount = 0; }
  double GetTimeStartedRunning_s() const { return _startedRunningTime_s; }
  double GetRunningDuration() const;
    
  // Will be called upon first switching to a behavior before calling update.
  // Calls protected virtual InitInternal() method, which each derived class
  // should implement.
  Result Init();

  // If this behavior is resuming after a short interruption (e.g. a cliff reaction), this Resume function
  // will be called instead. It calls ResumeInternal(), which defaults to simply calling InitInternal, but can
  // be implemented by children to do specific resume behavior (e.g. start at a specific state). If this
  // function returns anything other than RESULT_OK, the behavior will not be resumed (but may still be Init'd
  // later)
  Result Resume(ReactionTrigger resumingFromType);

  // Step through the behavior and deliver rewards to the robot along the way
  // This calls the protected virtual UpdateInternal() method, which each
  // derived class should implement.
  Status Update() ;
    
  // This behavior was the currently running behavior, but is now stopping (to make way for a new current
  // behavior). Any behaviors from StartActing will be canceled.
  void Stop();

  
  // Prevents the behavior from calling a callback function when ActionCompleted occurs
  // this allows the behavior to be stopped gracefully

  // NOTE: this may not actually stop the behavior. It will disable action callbacks, so if the behavior
  // relies on those to run (like almost all behaviors now do) it will stop. However, if the behavior isn't
  // using StartActing or is directly doing stuff from its UpdateInternal, this won't actually stop the
  // behavior
  void StopOnNextActionComplete();
  
  // Returns true if the state of the world/robot is sufficient for this behavior to be executed
  template<typename T>
  bool IsRunnable(const T& preReqData) const;

  
  const std::string& GetName() const { return _name; }
  const std::string& GetDisplayNameKey() const { return _displayNameKey; }
  const std::string& GetDebugStateName() const { return _debugStateName;}
  ExecutableBehaviorType GetExecutableType() const { return _executableType; }
  const BehaviorClass GetClass() const { return _behaviorClassID; }

  // Can be overridden to allow the behavior to run while the robot is not on it's treads (default is to not run)
  virtual bool ShouldRunWhileOffTreads() const { return false;}

  // Return true if the behavior explicitly handles the case where the robot starts holding the block
  // Equivalent to !robot.IsCarryingObject() in IsRunnable()
  virtual bool CarryingObjectHandledInternally() const = 0;

  bool IsOwnedByFactory() const { return _isOwnedByFactory; }
  
  bool IsBehaviorGroup(BehaviorGroup behaviorGroup) const { return _behaviorGroups.IsBitFlagSet(behaviorGroup); }
  bool MatchesAnyBehaviorGroups(const BehaviorGroupFlags::StorageType& flags) const {
    return _behaviorGroups.AreAnyBitsInMaskSet(flags);
  }
  bool MatchesAnyBehaviorGroups(const BehaviorGroupFlags& groupFlags) const {
    return MatchesAnyBehaviorGroups(groupFlags.GetFlags());
  }

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

  // Force a behavior to update its target blocks but only if it is in a state where it can
  void UpdateTargetBlocks(const Robot& robot) const { UpdateTargetBlocksInternal(robot); }
  
  // Get the ObjectUseIntentions this behavior uses
  virtual std::set<AIWhiteboard::ObjectUseIntention> GetBehaviorObjectUseIntentions() const { return {}; }
  
  // Handles stopping, updating target blocks, and re-initing the behavior so things get updated properly with
  // the newly tapped object
  // Returns whether or not the behavior can still run after updating target blocks
  bool HandleNewDoubleTap(Robot& robot);
  
  
  // Add Listeners to a behavior which will notify them of milestones/events in the behavior's lifecycle
  virtual void AddListener(ISubtaskListener* listener)
                { DEV_ASSERT(false, "AddListener.FrustrationListener.Unimplemented"); }
  virtual void AddListener(IReactToFaceListener* listener)
                { DEV_ASSERT(false, "AddListener.FaceListener.Unimplemented"); }
  virtual void AddListener(IReactToObjectListener* listener)
                { DEV_ASSERT(false, "AddListener.ObjectListener.Unimplemented"); }
  virtual void AddListener(IReactToPetListener* listener)
                { DEV_ASSERT(false, "AddListener.PetListener.Unimplemented"); }
  
protected:
  
  void ClearBehaviorGroups() { _behaviorGroups.ClearFlags(); }
  void SetBehaviorGroup(BehaviorGroup behaviorGroup, bool newVal = true) {
    _behaviorGroups.SetBitFlag(behaviorGroup, newVal);
  }
    
  // Only sets the name if it's currenty the base default name
  void SetDefaultName(const char* inName);
  inline void SetDebugStateName(const std::string& inName) {
    _debugStateName = inName;
    PRINT_CH_INFO("Behaviors", "Behavior.TransitionToState", "Behavior:%s, State:%s",
                  GetName().c_str(), _debugStateName.c_str());
  }
  
  inline void SetExecutableType(ExecutableBehaviorType type) { _executableType = type; }
    
  virtual Result InitInternal(Robot& robot) = 0;
  virtual Result ResumeInternal(Robot& robot);
  bool IsResuming() { return _isResuming;}

  // To keep passing through data generic, if robot is not overriden
  // check the NoPreReqs IsRunnableInternal
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData ) const
                 { BehaviorPreReqNone noPreReqs;  return IsRunnableInternal(noPreReqs);}
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData ) const
                 { DEV_ASSERT(false, "IsRunnableInternal.PreReqNone.NoOverride"); return false;}
  
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeObject& preReqData ) const
                 { DEV_ASSERT(false, "IsRunnableInternal.PreReqAcknowledgeObject.NoOverride"); return false;}
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeFace& preReqData ) const
                 { DEV_ASSERT(false, "IsRunnableInternal.PreReqAcknowledgeFace.NoOverride"); return false;}
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgePet& preReqData ) const
                 { DEV_ASSERT(false, "IsRunnableInternal.PreReqAcknowledgePet.NoOverride"); return false;}

  // This function can be implemented by behaviors. It should return Running while it is running, and Complete
  // or Failure as needed. If it returns Complete, Stop will be called. Default implementation is to
  // return Running while IsActing, and Complete otherwise
  virtual Status UpdateInternal(Robot& robot);
  virtual void   StopInternal(Robot& robot) { };

  Util::RandomGenerator& GetRNG() const;
    
  // Convenience aliases
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
  using RobotToEngineEvent= AnkiEvent<RobotInterface::RobotToEngine>;
  using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
  using GameToEngineTag   = ExternalInterface::MessageGameToEngineTag;
    
  // Derived classes should use these methods to subscribe to any tags they
  // are interested in handling.
  void SubscribeToTags(std::set<GameToEngineTag>&& tags);
  void SubscribeToTags(std::set<EngineToGameTag>&& tags);
  void SubscribeToTags(const uint32_t robotId, std::set<RobotInterface::RobotToEngineTag>&& tags);
    
  // Derived classes must override this method to handle events that come in
  // irrespective of whether the behavior is running or not. Note that the Robot
  // reference is const to prevent the behavior from modifying the robot when it
  // is not running. If the behavior is subscribed to multiple tags, the presumption
  // is that this will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called before HandleWhileRunning and HandleWhielNotRunning!
  virtual void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot) { }
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) { }
  virtual void AlwaysHandle(const RobotToEngineEvent& event, const Robot& robot) { }
    
  // Derived classes must override this method to handle events that come in
  // while the behavior is running. In this case, the behavior is allowed to
  // modify the robot and thus receives a non-const reference to it.
  // If the behavior is subscribed to multiple tags, the presumption is that it
  // will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called first!
  virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) { }
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) { }
  virtual void HandleWhileRunning(const RobotToEngineEvent& event, const Robot& robot) { }
    
  // Derived classes must override this method to handle events that come in
  // only while the behavior is NOT running. If it doesn't matter whether the
  // the behavior is running or not, consider using AlwaysHandle above instead.
  // If the behavior is subscribed to multiple tags, the presumption is that it
  // will handle switching based on tag internally.
  // NOTE: AlwaysHandle is called first!
  virtual void HandleWhileNotRunning(const GameToEngineEvent& event, const Robot& robot) { }
  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) { }
  virtual void HandleWhileNotRunning(const RobotToEngineEvent& event, const Robot& robot) { }
  
  // Many behaviors use a pattern of executing an action, then waiting for it to finish before selecting the
  // next action. Instead of directly starting actions and handling ActionCompleted callbacks, derived
  // classes can use the functions below. Note: none of the StartActing functions can be called when the
  // behavior is not running (will result in an error and no action). Also, if the behavior was running when
  // you called StartActing, but is no longer running when the action completed, the callback will NOT be
  // called (this is necessary to prevent non-running behaviors from doing things with the robot). If the
  // behavior is stopped, within the Stop function, any actions which are still running will be canceled
  // (and you will not get any callback for it).

  // Each StartActing function returns true if the action was started, false otherwise. Reasons actions
  // might not be started include:
  // 1. Another action (from StartActing) is currently running or queued
  // 2. You are not running ( IsRunning() returns false )

  // Start an action now, and optionally provide a callback which will be called with the
  // RobotCompletedAction that corresponds to the action
  using RobotCompletedActionCallback = std::function<void(const ExternalInterface::RobotCompletedAction&)>;
  bool StartActing(IActionRunner* action, RobotCompletedActionCallback callback = {});

  // helper that just looks at the result (simpler, but you can't get things like the completion union)
  using ActionResultCallback = std::function<void(ActionResult)>;
  bool StartActing(IActionRunner* action, ActionResultCallback callback);

  // If you want to do something when the action finishes, regardless of it's result, you can use the
  // following callbacks, with either a callback function taking no arguments or taking a single Robot&
  // argument. You can also use member functions. The callback will be called when action completes for any
  // reason (as long as the behavior is running)

  using SimpleCallback = std::function<void(void)>;
  bool StartActing(IActionRunner* action, SimpleCallback callback);

  using SimpleCallbackWithRobot = std::function<void(Robot& robot)>;
  bool StartActing(IActionRunner* action, SimpleCallbackWithRobot callback);

  template<typename T>
  bool StartActing(IActionRunner* action, void(T::*callback)(Robot&));

  template<typename T>
  bool StartActing(IActionRunner* action, void(T::*callback)(void));
  
  // This function cancels the action started by StartActing (if there is one). Returns true if an action
  // was canceled, false otherwise. Note that if you are running, this will trigger a callback for the
  // cancellation unless you set allowCallback to false
  bool StopActing(bool allowCallback = true);
  
  // Behaviors should call this function when they reach their completion state
  // in order to log das events and notify goal strategies if they listen for the message
  void BehaviorObjectiveAchieved(BehaviorObjective objectiveAchieved);
  
  // Allows the behavior to disable and enable reaction triggers without having to worry about re-enabling them
  // these triggers will be automatically re-enabled when the behavior stops
  void SmartDisableReactionTrigger(ReactionTrigger trigger);
  void SmartDisableReactionTrigger(const std::set<ReactionTrigger>& triggerList);
  
  // If a behavior needs to re-enable a reaction trigger for later stages after being
  // disabled with SmartDisablesableReactionaryBehavior  this function will re-enable the behavior
  // and stop tracking it
  void SmartReEnableReactionTrigger(ReactionTrigger trigger);
  void SmartReEnableReactionTrigger(const std::set<ReactionTrigger> triggerList);

  // Disable / ReEnable tap interaction. Useful if a behavior is concerned about trigger false double-taps
  void SmartDisableTapInteraction();
  void SmartReEnableTapInteraction();
  
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

  virtual void UpdateTargetBlocksInternal(const Robot& robot) const {};
  
  // Updates the double tapped object lights
  // If on == true will turn on the double tapped lights for the current tapped object
  // If on == false will clear double tapped lights
  void UpdateTappedObjectLights(const bool on) const;
  
  // Notify the audio controller that the behavior state has changed and the audio
  // should change to match
  // returns false if audio state not updated because behavior's audio counterpart
  // is not currently active
  bool UpdateAudioState(int newAudioState);
  
  // this process might be enabled/disabled automatically by a behavior chooser based
  // behavior specify it in code because it's a code requirement (not data driven)
  AIInformationAnalysis::EProcess _requiredProcess;
  
  bool RequiresObjectTapped() const { return _requireObjectTapped; }
  
  // Override if a behavior that uses double tapped objects needs to do something different when it stops
  // due to a double tap
  virtual void StopInternalFromDoubleTap(Robot& robot) { if(!RequiresObjectTapped()) { StopInternal(robot); } }
  
  inline void SetBehaviorClass(BehaviorClass classID) {if(_behaviorClassID == BehaviorClass::NoneBehavior){ _behaviorClassID = classID;}};

  //Allows behaviors to skip certain steps when streamlined
  //Can be set in json (for sparks) or programatically
  bool _shouldStreamline;
  
private:
  
  std::vector<::Signal::SmartHandle> _eventHandles;
  Robot& _robot;
  double _lastRunTime_s;
  double _startedRunningTime_s;
  
  // Returns true if the state of the world/robot is sufficient for this behavior to be executed
  bool IsRunnableBase(const Robot& robot) const;
  
  bool ReadFromJson(const Json::Value& config);
  
  template<class EventType>
  void HandleEvent(const EventType& event);

  // this is an internal handler just form StartActing
  void HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg);
  
  typedef std::set<ReactionTrigger>::iterator ReactionTriggerIter;
  // Used internally to delete 
  ReactionTriggerIter SmartReEnableReactionTrigger(ReactionTriggerIter iter);
  
    
  // ==================== Static Member Vars ====================
            
  static const char* kBaseDefaultName;
                
  // ==================== Member Vars ====================
    
  std::string _name;
  std::string _displayNameKey = "";
  std::string _debugStateName = "";
  BehaviorClass _behaviorClassID = BehaviorClass::NoneBehavior;
  ExecutableBehaviorType _executableType;
  
  // if an unlockId is set, the behavior won't be runnable unless the unlockId is unlocked in the progression component
  UnlockId _requiredUnlockId;
  
  
  // if _requiredRecentDriveOffCharger_sec is greater than 0, this behavior is only runnable if last time the robot got off the charger by
  // itself was less than this time ago. Eg, a value of 1 means if we got off the charger less than 1 second ago
  float _requiredRecentDriveOffCharger_sec;
  // if _requiredRecentSwitchToParent_sec is greater than 0, this behavior is only runnable if last time its parent behavior
  // chooser was activated happened less than this time ago. Eg: a value of 1 means 'if the parent got activated less
  // than 1 second ago'. This allows some behaviors to run only first time that their parent is activated (specially for goals)
  // TODO rsam: differentiate between (de)activation and interruption
  float _requiredRecentSwitchToParent_sec;
  
  int _startCount = 0;

  // for Start/StopActing if invalid, no action
  u32 _lastActionTag = ActionConstants::INVALID_TAG;
  RobotCompletedActionCallback  _actingCallback;
  bool _canStartActing = true;
    
  BehaviorGroupFlags  _behaviorGroups;

  bool _isRunning;
  // should only be used to allow StartActing to start while a behavior is resuming
  bool _isResuming;
  bool _isOwnedByFactory;
  
  const f32 kSamePreactionPoseDistThresh_mm = 30.f;
  const f32 kSamePreactionPoseAngleThresh_deg = 45.f;
  
  // A list of reactions that have been disabled at some point during the behavior
  // these will be automatically re-enabled during IBehavior::Stop using the current behavior's name
  // populated by SmartDisableReactionaryBehavior and SmartReEnableReactionaryBehavior
  std::set<ReactionTrigger> _disabledReactionTriggers;

  bool _tapInteractionDisabled = false;
  
  // An int that holds tracks disabled using SmartLockTrack
  std::map<std::string, u8> _lockingNameToTracksMap;
  
  bool _requireObjectTapped = false;

  //A list of object IDs that have had a custom light pattern set
  std::vector<ObjectID> _customLightObjects;
  
  
  
////////
//// Scored Behavior fuctions
///////
  
public:
  // Stops the behavaior immediately but gives it a couple of tick window during which score
  // evaluation will not include its running penalty.  This allows behaviors to
  // stop themselves in hopes of being re-selected with new fast-forwarding and
  // block settings without knocking its score down so something else is selected
  void StopWithoutImmediateRepetitionPenalty();
  
  
  float EvaluateScore(const Robot& robot) const;
  const MoodScorer& GetMoodScorer() const { return _moodScorer; }
  void ClearEmotionScorers()                         { _moodScorer.ClearEmotionScorers(); }
  void AddEmotionScorer(const EmotionScorer& scorer) { _moodScorer.AddEmotionScorer(scorer); }
  size_t GetEmotionScorerCount() const { return _moodScorer.GetEmotionScorerCount(); }
  const EmotionScorer& GetEmotionScorer(size_t index) const { return _moodScorer.GetEmotionScorer(index); }
  
  float EvaluateRepetitionPenalty() const;
  const Util::GraphEvaluator2d& GetRepetitionPenalty() const { return _repetitionPenalty; }
  
  float EvaluateRunningPenalty() const;
  const Util::GraphEvaluator2d& GetRunningPenalty() const { return _runningPenalty; }

protected:
  
  virtual void ScoredActingStateChanged(bool isActing);
  
  // EvaluateScoreInternal is used to score each behavior for behavior selection - it uses mood scorer or
  // flat score depending on configuration. If the behavior is running, it uses the Running score to decide if it should
  // keep running
  virtual float EvaluateRunningScoreInternal(const Robot& robot) const;
  virtual float EvaluateScoreInternal(const Robot& robot) const;
  
  // Additional start acting definitions realiting to score
  
  // Called after StartActing, will add extraScore to the result of EvaluateRunningScoreInternal. This makes
  // it easy to encourage the system to keep a behavior running while it is acting. NOTE: multiple calls to
  // this function (for the same action) will be cumulative. The bonus will be reset as soon as the action is
  // complete, or the behavior is no longer running
  void IncreaseScoreWhileActing(float extraScore);
  
  // Convenience wrappers that combine IncreaseScoreWhileActing with StartActing,
  // including any of the callback types above
  virtual bool StartActingExtraScore(IActionRunner* action, float extraScoreWhileActing) final;
  
  template<typename CallbackType>
  bool StartActingExtraScore(IActionRunner* action, float extraScoreWhileActing, CallbackType callback);
  
  bool IsRunnableScored(const BehaviorPreReqNone& preReqData) const;
  
private:
  void ScoredConstructor(Robot& robot);
  void InitScored(Robot& robot);
  
  bool ReadFromScoredJson(const Json::Value& config);
  
  void HandleBehaviorObjective(const ExternalInterface::BehaviorObjectiveAchieved& msg);
  
  // ==================== Member Vars ====================
  MoodScorer              _moodScorer;
  Util::GraphEvaluator2d  _repetitionPenalty;
  Util::GraphEvaluator2d  _runningPenalty;
  float                   _flatScore = 0;
  float                   _extraRunningScore = 0;
  
  // used to allow short times during which repetitionPenalties don't apply
  float                   _nextTimeRepetitionPenaltyApplies_s = 0;
  
  // if this behavior objective gets sent (by any behavior), then consider this behavior to have been running
  // (for purposes of repetition penalty, aka cooldown)
  BehaviorObjective _cooldownOnObjective = BehaviorObjective::Count;
  
  
  bool _enableRepetitionPenalty = true;
  bool _enableRunningPenalty = true;
  
  // Keep track of the number of times resumed from
  int _timesResumedFromPossibleInfiniteLoop = 0;
  float _timeCanRunAfterPossibleInfiniteLoopCooldown_sec = 0;
  
}; // class IBehavior
  

template<typename T>
bool IBehavior::IsRunnable(const T& preReqData) const
{
  if(IsRunnableBase(_robot)){
    return IsRunnableInternal(preReqData);
  }
  
  return false;
}
  
  
template<typename T>
bool IBehavior::StartActing(IActionRunner* action, void(T::*callback)(Robot&))
{
  return StartActing(action, std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}

template<typename T>
bool IBehavior::StartActing(IActionRunner* action, void(T::*callback)(void))
{
  std::function<void(void)> boundCallback = std::bind(callback, static_cast<T*>(this));
  return StartActing(action, boundCallback);
}

inline bool IBehavior::IsActing() const {
  return _lastActionTag != ActionConstants::INVALID_TAG;
}


  
template<class EventType>
void IBehavior::HandleEvent(const EventType& event)
{
  AlwaysHandle(event, _robot);
    
  if(IsRunning()) {
    HandleWhileRunning(event, _robot);
  } else {
    HandleWhileNotRunning(event, _robot);
  }
}
  

////////
//// Scored Behavior fuctions
///////
  
inline bool IBehavior::StartActingExtraScore(IActionRunner* action, float extraScoreWhileActing) {
  IncreaseScoreWhileActing(extraScoreWhileActing);
  return StartActing(action);
  return true;
}

template<typename CallbackType>
inline bool IBehavior::StartActingExtraScore(IActionRunner* action, float extraScoreWhileActing, CallbackType callback) {
  IncreaseScoreWhileActing(extraScoreWhileActing);
  return StartActing(action, callback);
}

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_IBehavior_H__
