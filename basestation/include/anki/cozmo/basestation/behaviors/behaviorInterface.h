/**
 * File: behaviorInterface.h
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines interface for a Cozmo "Behavior".
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorInterface_H__
#define __Cozmo_Basestation_Behaviors_BehaviorInterface_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupFlags.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"
#include "anki/cozmo/basestation/moodSystem/emotionScorer.h"
#include "anki/cozmo/basestation/moodSystem/moodScorer.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "json/json-forwards.h"
#include <set>

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "clad/types/behaviorGroup.h"
#include "clad/types/behaviorObjectives.h"
#include "clad/types/behaviorTypes.h"
#include "clad/types/unlockTypes.h"
#include "util/bitFlags/bitFlags.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"


//Transforms enum into string
#define DEBUG_SET_STATE(s) SetDebugStateName(#s)

// This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
// All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
  if ((_BEHAVIORDEF)) { PRINT_NAMED_INFO( __VA_ARGS__ ); } \
  else { PRINT_NAMED_DEBUG( __VA_ARGS__ ); } \
  } while(0) \

namespace Anki {
namespace Cozmo {
  
// Forward declarations
class IReactionaryBehavior;
class MoodManager;
class Robot;
class Reward;
class ActionableObject;
class DriveToObjectAction;

namespace ExternalInterface {
class MessageEngineToGame;
class MessageGameToEngine;
struct RobotCompletedAction;
}
template<typename TYPE> class AnkiEvent;
  
// Base Behavior Interface specification
class IBehavior
{
protected:
    
  // Enforce creation through BehaviorFactory
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

  // returns the number of times this behavior has been started (number of times Init was called and returned
  // OK, not counting calls to Resume)
  int GetNumTimesBehaviorStarted() const { return _startCount; }
  void ResetStartCount() { _startCount = 0; }
    
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
  Result Resume();  

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
  
  //
  // Abstract methods to be overloaded:
  //
    
  // Returns true iff the state of the world/robot is sufficient for this
  // behavior to be executed
  bool IsRunnable(const Robot& robot) const;
  
  const std::string& GetName() const { return _name; }
  const std::string& GetDebugStateName() const { return _debugStateName;}
  const BehaviorType GetType() const { return _behaviorType; }
  virtual bool IsReactionary() const { return false;}
  virtual bool ShouldRunWhileOffTreads() const { return false;}
    
  // Return true if the behavior explicitly handles the case where the robot starts holding the block
  // Equivalent to !robot.IsCarryingObject() in IsRunnable()
  virtual bool CarryingObjectHandledInternally() const = 0;

  double GetTimeStartedRunning_s() const { return _startedRunningTime_s; }

  // returns true if any action from StartAction is currently running, indicating that the behavior is
  // likely waiting for something to complete
  inline bool IsActing() const;
    
  float EvaluateScore(const Robot& robot) const;

  const MoodScorer& GetMoodScorer() const { return _moodScorer; }
    
  void ClearEmotionScorers()                         { _moodScorer.ClearEmotionScorers(); }
  void AddEmotionScorer(const EmotionScorer& scorer) { _moodScorer.AddEmotionScorer(scorer); }
  size_t GetEmotionScorerCount() const { return _moodScorer.GetEmotionScorerCount(); }
  const EmotionScorer& GetEmotionScorer(size_t index) const { return _moodScorer.GetEmotionScorer(index); }
    
  
  float EvaluateRepetitionPenalty() const;
  const Util::GraphEvaluator2d& GetRepetionalPenalty() const { return _repetitionPenalty; }

  float EvaluateRunningPenalty() const;
  const Util::GraphEvaluator2d& GetRunningPenalty() const { return _runningPenalty; }

  bool IsOwnedByFactory() const { return _isOwnedByFactory; }
    
  virtual IReactionaryBehavior* AsReactionaryBehavior() { assert(0); return nullptr; }
  
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
  inline void SetBehaviorType(BehaviorType type) {if(_behaviorType == BehaviorType::NoneBehavior){ _behaviorType = type;}};
    
  virtual Result InitInternal(Robot& robot) = 0;
  virtual Result ResumeInternal(Robot& robot);
  virtual bool IsRunnableInternal(const Robot& robot) const = 0;

  // EvaluateScoreInternal is used to score each behavior for behavior selection - it uses mood scorer or
  // flat score depending on configuration. If the behavior is running, it uses the Running score to decide if it should
  // keep running
  virtual float EvaluateRunningScoreInternal(const Robot& robot) const;
  virtual float EvaluateScoreInternal(const Robot& robot) const;

  // This function can be implemented by behaviors. It should return Running while it is running, and Complete
  // or Failure as needed. If it returns Complete, Stop will be called. Default implementation is to
  // return Running while IsActing, and Complete otherwise
  virtual Status UpdateInternal(Robot& robot);
  virtual void   StopInternal(Robot& robot) { };
    
  bool ReadFromJson(const Json::Value& config);

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

  // Called after StartActing, will add extraScore to the result of EvaluateRunningScoreInternal. This makes
  // it easy to encourage the system to keep a behavior running while it is acting. NOTE: multiple calls to
  // this function (for the same action) will be cumulative. The bonus will be reset as soon as the action is
  // complete, or the behavior is no longer running
  void IncreaseScoreWhileActing(float extraScore);
  
  // Convenience wrappers that combine IncreaseScoreWhileActing with StartActing,
  // including any of the callback types above
  bool StartActing(IActionRunner* action, float extraScoreWhileActing);
  
  template<typename CallbackType>
  bool StartActing(IActionRunner* action, float extraScoreWhileActing, CallbackType callback);
  
  // This function cancels the action started by StartActing (if there is one). Returns true if an action
  // was canceled, false otherwise. Note that if you are running, this will trigger a callback for the
  // cancellation unless you set allowCallback to false
  bool StopActing(bool allowCallback = true);
  
  // Behaviors should call this function when they reach their completion state
  // in order to log das events and notify goal strategies if they listen for the message
  void BehaviorObjectiveAchieved(BehaviorObjective objectiveAchieved);
  
  //Allows behaviors to skip certain steps when streamlined
  //Can be set in json (for sparks) or programatically
  bool _shouldStreamline;

private:
            
  template<class EventType>
  void HandleEvent(const EventType& event);

  // this is an internal handler just form StartActing
  void HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg);
    
  // ==================== Static Member Vars ====================
            
  static const char* kBaseDefaultName;
                
  // ==================== Member Vars ====================
    
  std::string _name;
  std::string _debugStateName = "";
  BehaviorType _behaviorType;
  
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
  
  MoodScorer              _moodScorer;
  Util::GraphEvaluator2d  _repetitionPenalty;
  Util::GraphEvaluator2d  _runningPenalty;
  float                   _flatScore;
  
  Robot& _robot;
    
  std::vector<::Signal::SmartHandle> _eventHandles;
    
  double _startedRunningTime_s;
  double _lastRunTime_s;
  int _startCount = 0;

  // for Start/StopActing if invalid, no action
  u32 _lastActionTag = ActionConstants::INVALID_TAG;
  RobotCompletedActionCallback  _actingCallback;
  float _extraRunningScore;
  bool _canStartActing = true;
    
  BehaviorGroupFlags  _behaviorGroups;

  bool _isRunning;
  bool _isOwnedByFactory;
  
  bool _enableRepetitionPenalty;
  bool _enableRunningPenalty;
  
  const f32 kSamePreactionPoseDistThresh_mm = 30.f;
  const f32 kSamePreactionPoseAngleThresh_deg = 45.f;
    
}; // class IBehavior
  
  
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

inline bool IBehavior::StartActing(IActionRunner* action, float extraScoreWhileActing) {
  IncreaseScoreWhileActing(extraScoreWhileActing);
  return StartActing(action);
}

template<typename CallbackType>
inline bool IBehavior::StartActing(IActionRunner* action, float extraScoreWhileActing, CallbackType callback) {
  IncreaseScoreWhileActing(extraScoreWhileActing);
  return StartActing(action, callback);
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
  
  
class IReactionaryBehavior : public IBehavior
{
protected:
    
  // Enforce creation through BehaviorFactory
  IReactionaryBehavior(Robot& robot, const Json::Value& config);
  virtual ~IReactionaryBehavior() {}
    
public:

  virtual const std::set<EngineToGameTag>& GetEngineToGameTags() const { return _engineToGameTags; }
  virtual const std::set<GameToEngineTag>& GetGameToEngineTags() const { return _gameToEngineTags; }
  virtual const std::set<RobotInterface::RobotToEngineTag>& GetRobotToEngineTags() const { return _robotToEngineTags; }
    
  // Subscribe to tags that should immediately trigger this behavior
  void SubscribeToTriggerTags(std::set<EngineToGameTag>&& tags);
  void SubscribeToTriggerTags(std::set<GameToEngineTag>&& tags);
  void SubscribeToTriggerTags(const uint32_t robotId, std::set<RobotInterface::RobotToEngineTag>&& tags);
  
  //Handle enabling/disabling of reactionary behaviors
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) final override;
  virtual void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot) final override;
  virtual void AlwaysHandle(const RobotToEngineEvent& event, const Robot& robot) final override;
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot){};
  virtual void AlwaysHandleInternal(const GameToEngineEvent& event, const Robot& robot){};
  virtual void AlwaysHandleInternal(const RobotToEngineEvent& event, const Robot& robot){};
  
  // if a trigger tag is received, this function will be called. If it returns true, this behavior will run
  // immediately
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) { return true; }
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageGameToEngine& event, const Robot& robot) { return true; }
  virtual bool ShouldRunForEvent(const RobotInterface::RobotToEngine& event, const Robot& robot) { return true; }
  
  // behavior manager checks the return value of this function every tick
  // to see if the reactionary behavior has requested a computational switch
  // override to trigger a reactionary behavior based on something other than a message
  virtual bool ShouldComputationallySwitch(const Robot& robot){ return false;}

  // if this returns false, then don't start this behavior if another reactionary is already running
  virtual bool ShouldInterruptOtherReactionaryBehavior() { return true; }
  
  virtual IReactionaryBehavior* AsReactionaryBehavior() override { return this; }

  virtual bool IsReactionary() const override { return true;}
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
  //Deal with default disabling - has to be called after type is set for behavior
  virtual void HandleDisableByDefault(Robot& robot);
  
  // if true, the previously running behavior will be resumed (if possible) after this behavior is
  // complete. Otherwise, a new behavior will be selected by the chooser after this one runs
  virtual bool ShouldResumeLastBehavior() const = 0;
      
protected:

  virtual Result InitInternal(Robot& robot) override final;
  virtual void   StopInternal(Robot& robot) override final;
  
  //Never Called - reactionary behaviors don't resume
  virtual Result ResumeInternal(Robot& robot) override final;
  
  virtual Result InitInternalReactionary(Robot& robot) = 0;
  virtual void   StopInternalReactionary(Robot& robot){};

  bool IsReactionEnabled() const { return _disableIDs.empty(); }
  
  void LoadConfig(Robot& robot, const Json::Value& config);
  
  //Handle tracking enable/disable requests
  virtual void UpdateDisableIDs(std::string& requesterID, bool enable);

  virtual bool IsRunnableInternal(const Robot& robot) const override final;
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const = 0;

  std::multiset<std::string> _disableIDs;
  std::set<EngineToGameTag> _engineToGameTags;
  std::set<GameToEngineTag> _gameToEngineTags;
  std::set<RobotInterface::RobotToEngineTag> _robotToEngineTags;
  bool _isDisabledByDefault;

}; // class IReactionaryBehavior

template<typename EventTag>
inline const std::set<EventTag>& GetReactionaryBehaviorTags(const IReactionaryBehavior* behavior);

////////////////////////////////////////////////////////////////////////////////

template<>
inline const std::set<ExternalInterface::MessageEngineToGameTag>&
GetReactionaryBehaviorTags<ExternalInterface::MessageEngineToGameTag>(const IReactionaryBehavior* behavior)
{
  return behavior->GetEngineToGameTags();
}

template<>
inline const std::set<ExternalInterface::MessageGameToEngineTag>&
GetReactionaryBehaviorTags<ExternalInterface::MessageGameToEngineTag>(const IReactionaryBehavior* behavior)
{
  return behavior->GetGameToEngineTags();
}
  
template<>
inline const std::set<RobotInterface::RobotToEngineTag>&
GetReactionaryBehaviorTags<RobotInterface::RobotToEngineTag>(const IReactionaryBehavior* behavior)
{
  return behavior->GetRobotToEngineTags();
}


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorInterface_H__
