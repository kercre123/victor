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

#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupFlags.h"
#include "anki/cozmo/basestation/moodSystem/moodScorer.h"
#include "anki/cozmo/basestation/moodSystem/emotionScorer.h"
#include "util/random/randomGenerator.h"
#include "json/json.h"
#include <set>

#include "util/bitFlags/bitFlags.h"
#include "util/logging/logging.h"
#include "util/signals/simpleSignal_fwd.h"
#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/types/behaviorGroup.h"

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
  namespace ExternalInterface {
    class MessageEngineToGame;
    class MessageGameToEngine;
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
    
    // BehaviorManager uses SetIsRunning() when it starts or stops a behavior.
    // IsRunning() allows querying from inside a subclass to do things differently
    // (such as handling events) depending on running state.
    bool IsRunning() const { return _isRunning; }
    void SetIsRunning(bool tf) { _isRunning = tf; }
    
    // Will be called upon first switching to a behavior before calling update.
    // Calls protected virtual InitInternal() method, which each derived class
    // should implement.
    Result Init(double currentTime_sec, bool isResuming);

    // Step through the behavior and deliver rewards to the robot along the way
    // This calls the protected virtual UpdateInternal() method, which each
    // derived class should implement.
    Status Update(double currentTime_sec) ;
    
    // This behavior was the currently running behavior, but is now stopping (to make way for a new current behavior)
    void Stop(double currentTime_sec);
    
    //
    // Abstract methods to be overloaded:
    //
    
    // Returns true iff the state of the world/robot is sufficient for this
    // behavior to be executed
    virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const = 0;
    
    // Tell this behavior to finish up ASAP so we can switch to a new one.
    // This should trigger any cleanup and get Update() to return COMPLETE
    // as quickly as possible.
    Result Interrupt(double currentTime_sec, bool isShortInterrupt);
    
    const std::string& GetName() const { return _name; }
    const std::string& GetStateName() const { return _stateName; }
    
    // EvaluateEmotionScore is a score directly based on the given emotion rules
    float EvaluateEmotionScore(const MoodManager& moodManager) const;
    
    // EvaluateScoreInternal is used to score each behavior for behavior selection - by default it just uses EvaluateEmotionScore
    virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const;
    
    float EvaluateScore(const Robot& robot, double currentTime_sec) const;

    const MoodScorer& GetMoodScorer() const { return _moodScorer; }
    
    void ClearEmotionScorers()                         { _moodScorer.ClearEmotionScorers(); }
    void AddEmotionScorer(const EmotionScorer& scorer) { _moodScorer.AddEmotionScorer(scorer); }
    size_t GetEmotionScorerCount() const { return _moodScorer.GetEmotionScorerCount(); }
    const EmotionScorer& GetEmotionScorer(size_t index) const { return _moodScorer.GetEmotionScorer(index); }
    
    
    void SetOverrideScore(float newVal) { _overrideScore = newVal; }
    
    float EvaluateRepetitionPenalty(double currentTime_sec) const;
    const Util::GraphEvaluator2d& GetRepetionalPenalty() const { return _repetitionPenalty; }
    
    bool IsOwnedByFactory() const { return _isOwnedByFactory; }
    
    // Some behaviors are short interruptions that can resume directly to previous behavior
    bool IsShortInterruption() const { return IsBehaviorGroup(BehaviorGroup::ShortInterruption); }
    virtual bool WantsToResume() const { return false; }
    
    // All behaviors run in a single "slot" in the AcitonList. (This seems icky.)
    static const ActionList::SlotHandle sActionSlot;
    
    virtual IReactionaryBehavior* AsReactionaryBehavior() { assert(0); return nullptr; }
    
    bool IsBehaviorGroup(BehaviorGroup behaviorGroup) const { return _behaviorGroups.IsBitFlagSet(behaviorGroup); }
    bool MatchesAnyBehaviorGroups(BehaviorGroupFlags::StorageType flags) const { return _behaviorGroups.AreAnyBitsInMaskSet(flags); }
    bool MatchesAnyBehaviorGroups(const BehaviorGroupFlags& groupFlags) const { return MatchesAnyBehaviorGroups(groupFlags.GetFlags()); }
    
  protected:
  
    void ClearBehaviorGroups() { _behaviorGroups.ClearFlags(); }
    void SetBehaviorGroup(BehaviorGroup behaviorGroup, bool newVal = true) { _behaviorGroups.SetBitFlag(behaviorGroup, newVal); }
    
    // Going forward we don't want names being set arbitrarily (they can come from data etc.)
    void DEMO_HACK_SetName(const char* inName) { _name = inName; }
    // Only sets the name if it's currenty the base default name
    inline void SetDefaultName(const char* inName);
    inline void SetStateName(const std::string& inName) { _stateName = inName; }
    
    virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) = 0;
    virtual Status UpdateInternal(Robot& robot, double currentTime_sec) = 0;
    virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) = 0;
    virtual void   StopInternal(Robot& robot, double currentTime_sec) = 0;
    
    bool ReadFromJson(const Json::Value& config);
    
    // Convenience aliases
    using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
    using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
    using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
    using GameToEngineTag   = ExternalInterface::MessageGameToEngineTag;
    
    // Derived classes should use these methods to subscribe to any tags they
    // are interested in handling.
    void SubscribeToTags(std::set<GameToEngineTag>&& tags);
    void SubscribeToTags(std::set<EngineToGameTag>&& tags);
    
    // Derived classes must override this method to handle events that come in
    // irrespective of whether the behavior is running or not. Note that the Robot
    // reference is const to prevent the behavior from modifying the robot when it
    // is not running. If the behavior is subscribed to multiple tags, the presumption
    // is that this will handle switching based on tag internally.
    // NOTE: AlwaysHandle is called before HandleWhileRunning and HandleWhielNotRunning!
    virtual void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot) { }
    virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) { }
    
    // Derived classes must override this method to handle events that come in
    // while the behavior is running. In this case, the behavior is allowed to
    // modify the robot and thus receives a non-const reference to it.
    // If the behavior is subscribed to multiple tags, the presumption is that it
    // will handle switching based on tag internally.
    // NOTE: AlwaysHandle is called first!
    virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) { }
    virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) { }
    
    // Derived classes must override this method to handle events that come in
    // only while the behavior is NOT running. If it doesn't matter whether the
    // the behavior is running or not, consider using AlwaysHandle above instead.
    // If the behavior is subscribed to multiple tags, the presumption is that it
    // will handle switching based on tag internally.
    // NOTE: AlwaysHandle is called first!
    virtual void HandleWhileNotRunning(const GameToEngineEvent& event, const Robot& robot) { }
    virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) { }
    
    Util::RandomGenerator& GetRNG() const;
    
  private:
            
    template<class EventType>
    void HandleEvent(const EventType& event);

    // ==================== Static Member Vars ====================
            
    static const char* kBaseDefaultName;
                
    // ==================== Member Vars ====================
    
    std::string _name;
    std::string _stateName = "";
        
    MoodScorer                 _moodScorer;
    Util::GraphEvaluator2d     _repetitionPenalty;
    
    Robot& _robot;
    
    std::vector<::Signal::SmartHandle> _eventHandles;
    
    double _lastRunTime;

    float _overrideScore; // any value >= 0 implies it should be used
    
    BehaviorGroupFlags  _behaviorGroups;

    bool _isRunning;
    bool _isOwnedByFactory;
    
    bool _enableRepetitionPenalty;
    
  }; // class IBehavior
  
  inline Result IBehavior::Init(double currentTime_sec, bool isResuming)
  {
    return InitInternal(_robot, currentTime_sec, isResuming);
  }
  
  inline IBehavior::Status IBehavior::Update(double currentTime_sec)
  {
    ASSERT_NAMED(IsRunning(), "IBehavior::UpdateNotRunning");
    return UpdateInternal(_robot, currentTime_sec);
  }

  inline void IBehavior::Stop(double currentTime_sec)
  {
    StopInternal(_robot, currentTime_sec);
    _lastRunTime = currentTime_sec;
  }
  
  inline Result IBehavior::Interrupt(double currentTime_sec, bool isShortInterrupt)
  {
    return InterruptInternal(_robot, currentTime_sec, isShortInterrupt);
  }
  
  inline Util::RandomGenerator& IBehavior::GetRNG() const {
    static Util::RandomGenerator sRNG;
    return sRNG;
  }
  
  inline void IBehavior::SetDefaultName(const char* inName)
  {
    if (_name == kBaseDefaultName)
    {
      _name = inName;
    }
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
    
    // Subscribe to tags that should immediately trigger this behavior
    void SubscribeToTriggerTags(std::set<EngineToGameTag>&& tags);
    void SubscribeToTriggerTags(std::set<GameToEngineTag>&& tags);
    
    virtual IReactionaryBehavior* AsReactionaryBehavior() override { return this; }
    
  protected:
    std::set<EngineToGameTag> _engineToGameTags;
    std::set<GameToEngineTag> _gameToEngineTags;
  }; // class IReactionaryBehavior


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorInterface_H__
