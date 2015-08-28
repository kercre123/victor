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
#include "util/random/randomGenerator.h"
#include "json/json.h"
#include <set>

// This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
// All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
  if ((_BEHAVIORDEF)) { PRINT_NAMED_INFO( __VA_ARGS__ ); } \
  else { PRINT_NAMED_DEBUG( __VA_ARGS__ ); } \
  } while(0) \

namespace Anki {
namespace Cozmo {
  
  // Forward declarations
  class Robot;
  class Reward;
  namespace ExternalInterface {
    enum class MessageEngineToGameTag : uint8_t;
    enum class MessageGameToEngineTag : uint8_t;
  }
  
  // Base Behavior Interface specification
  class IBehavior
  {
  public:
    enum class Status {
      Failure,
      Running,
      Complete
    };
    
    IBehavior(Robot& robot, const Json::Value& config);
    virtual ~IBehavior() { }
    
    // BehaviorManager uses SetIsRunning() when it starts or stops a behavior.
    // IsRunning() allows querying from inside a subclass to do things differently
    // (such as handling events) depending on running state.
    bool IsRunning() const { return _isRunning; }
    void SetIsRunning(bool tf) { _isRunning = tf; }
    
    //
    // Abstract methods to be overloaded:
    //
    
    // Returns true iff the state of the world/robot is sufficient for this
    // behavior to be executed
    virtual bool IsRunnable(float currentTime_sec) const = 0;
    
    // Will be called upon first switching to a behavior before calling update.
    virtual Result Init() = 0;
    
    // Step through the behavior and deliver rewards to the robot along the way
    virtual Status Update(float currentTime_sec) = 0;
    
    // Tell this behavior to finish up ASAP so we can switch to a new one.
    // This should trigger any cleanup and get Update() to return COMPLETE
    // as quickly as possible.
    virtual Result Interrupt(float currentTime_sec) = 0;
    
    // Figure out the reward this behavior will offer, given the robot's current
    // state. Returns true if the Behavior is runnable, false if not. (In the
    // latter case, the returned reward is not populated.)
    virtual bool GetRewardBid(Reward& reward) = 0;
    
    virtual const std::string& GetName() const { return _name; }
    virtual const std::string& GetStateName() const { return _stateName; }
    
    // All behaviors run in a single "slot" in the AcitonList. (This seems icky.)
    static const ActionList::SlotHandle sActionSlot;
    
  protected:
    
    Robot &_robot;
    
    // A random number generator for all behaviors to share
    Util::RandomGenerator _rng;
    
    std::string _name = "no_name";
    std::string _stateName = "";
    
  private:
    
    bool _isRunning;
    
  }; // class IBehavior
  
  
  class IReactionaryBehavior : public IBehavior
  {
  public:
    using EngineToGameTag = ExternalInterface::MessageEngineToGameTag;
    using GameToEngineTag = ExternalInterface::MessageGameToEngineTag;
    
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