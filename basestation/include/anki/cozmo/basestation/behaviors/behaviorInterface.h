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

#include "anki/cozmo/basestation/actionContainers.h"
#include "anki/cozmo/basestation/moodSystem/emotionScorer.h"
#include "util/random/randomGenerator.h"
#include "json/json.h"
#include <set>

#include "util/signals/simpleSignal_fwd.h"
#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"

// This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
// All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
  if ((_BEHAVIORDEF)) { PRINT_NAMED_INFO( __VA_ARGS__ ); } \
  else { PRINT_NAMED_DEBUG( __VA_ARGS__ ); } \
  } while(0) \

namespace Anki {
namespace Cozmo {
  
  // Forward declarations
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
  public:
    enum class Status {
      Failure,
      Running,
      Complete
    };
    
    virtual ~IBehavior() { }
    
    // BehaviorManager uses SetIsRunning() when it starts or stops a behavior.
    // IsRunning() allows querying from inside a subclass to do things differently
    // (such as handling events) depending on running state.
    bool IsRunning() const { return _isRunning; }
    void SetIsRunning(bool tf) { _isRunning = tf; }
    
    // Will be called upon first switching to a behavior before calling update.
    // Calls protected virtual InitInternal() method, which each derived class
    // should implement.
    Result Init(double currentTime_sec);

    // Step through the behavior and deliver rewards to the robot along the way
    // This calls the protected virtual UpdateInternal() method, which each
    // derived class should implement.
    Status Update(double currentTime_sec) ;
    
    //
    // Abstract methods to be overloaded:
    //
    
    // Returns true iff the state of the world/robot is sufficient for this
    // behavior to be executed
    virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const = 0;
    
    // Tell this behavior to finish up ASAP so we can switch to a new one.
    // This should trigger any cleanup and get Update() to return COMPLETE
    // as quickly as possible.
    Result Interrupt(double currentTime_sec);
    
    virtual const std::string& GetName() const { return _name; }
    virtual const std::string& GetStateName() const { return _stateName; }
    
    // EvaluateEmotionScore is a score directly based on the given emotion rules
    float EvaluateEmotionScore(const MoodManager& moodManager) const;
    
    // EvaluateScore is used to score each behavior for behavior selection - by default it just uses EvaluateEmotionScore
    virtual float EvaluateScore(const Robot& robot, double currentTime_sec) const;

    void ClearEmotionScorers()                         { _emotionScorers.clear(); }
    void AddEmotionScorer(const EmotionScorer& scorer) { _emotionScorers.push_back(scorer); }
    
    // All behaviors run in a single "slot" in the AcitonList. (This seems icky.)
    static const ActionList::SlotHandle sActionSlot;
    
  protected:
    
    virtual Result InitInternal(Robot& robot, double currentTime_sec) = 0;
    virtual Status UpdateInternal(Robot& robot, double currentTime_sec) = 0;
    virtual Result InterruptInternal(Robot& robot, double currentTime_sec) = 0;
    
    // Can't create a public IBehavior, but derived classes must pass a robot
    // reference into this protected constructor.
    IBehavior(Robot& robot, const Json::Value& config);
    
    // Convenience aliases
    using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
    using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
    using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
    using GameToEngineTag   = ExternalInterface::MessageGameToEngineTag;
    
    // Derived classes should use these methods to subscribe to any tags they
    // are interested in handling.
    void SubscribeToTags(std::vector<GameToEngineTag>&& tags);
    void SubscribeToTags(std::vector<EngineToGameTag>&& tags);
    
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
    
    Util::RandomGenerator& GetRNG();
    
    std::string _name = "no_name";
    std::string _stateName = "";
    
    std::vector<EmotionScorer> _emotionScorers;
    
  private:
    
    Robot& _robot;
    
    bool _isRunning;
    
    // A random number generator for all behaviors to share
    static Util::RandomGenerator sRNG;

    std::vector<::Signal::SmartHandle> _eventHandles;
    
    template<class EventType>
    void HandleEvent(const EventType& event);
    
  }; // class IBehavior
  
  inline Result IBehavior::Init(double currentTime_sec)
  {
    return InitInternal(_robot, currentTime_sec);
  }
  
  inline IBehavior::Status IBehavior::Update(double currentTime_sec)
  {
    return UpdateInternal(_robot, currentTime_sec);
  }
  
  inline Result IBehavior::Interrupt(double currentTime_sec)
  {
    return InterruptInternal(_robot, currentTime_sec);
  }
  
  inline Util::RandomGenerator& IBehavior::GetRNG() {
    return sRNG;
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
  public:
    
    IReactionaryBehavior(Robot& robot, const Json::Value& config) : IBehavior(robot, config) { }
    
    virtual const std::set<EngineToGameTag>& GetEngineToGameTags() const { return _engineToGameTags; }
    virtual const std::set<GameToEngineTag>& GetGameToEngineTags() const { return _gameToEngineTags; }
    
  protected:
    std::set<EngineToGameTag> _engineToGameTags;
    std::set<GameToEngineTag> _gameToEngineTags;
  }; // class IReactionaryBehavior

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorInterface_H__