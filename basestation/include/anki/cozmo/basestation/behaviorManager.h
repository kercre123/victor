/**
 * File: behaviorManager.h
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 *
 * Overhaul: 7/30/15, Andrew Stein
 *
 * Description: High-level module that is a container for available behaviors,
 *              determines what behavior a robot should be executing at a given
 *              time and handles ticking the running behavior forward.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_BEHAVIOR_MANAGER_H
#define COZMO_BEHAVIOR_MANAGER_H

#include "anki/common/types.h"

#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"

#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"

#include "json/json.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <random>
#include <vector>

namespace Anki {
namespace Cozmo {
  
// Forward declaration
class IBehavior;
class IReactionaryBehavior;
class IBehaviorChooser;
class BehaviorFactory;
class Reward;
class Robot;
class BehaviorWhiteboard;
  
namespace ExternalInterface {
class BehaviorManagerMessageUnion;
}
  
class BehaviorManager
{
public:
    
  BehaviorManager(Robot& robot);
  ~BehaviorManager();
    
  Result Init(const Json::Value& config);
    
  // Calls the currently-selected behavior's Update() method until it
  // returns COMPLETE or FAILURE. Once current behavior completes
  // switches to next behavior (including an "idle" behavior?).
  Result Update();
    
  // Picks next behavior based on robot's current state. This does not
  // transition immediately to running that behavior, but will let the
  // current beheavior know it needs wind down with a call to its
  // Interrupt() method.
  Result SelectNextBehavior();
    
  // Forcefully select the next behavior by name (versus by letting the
  // selection mechanism choose based on current state). Fails if that
  // behavior does not exist or the selected behavior is not runnable.
  Result SelectNextBehavior(const std::string& name);
    
  // Specify the minimum time we should stay in each behavior before
  // considering switching
  void SetMinBehaviorTime(float time_sec) { _minBehaviorTime_sec = time_sec; }
    
  // Set the current IBehaviorChooser. Note this results in the destruction of
  // the current IBehaviorChooser (if there is one) and the IBehaviors it contains
  void SetBehaviorChooser(IBehaviorChooser* newChooser);
    
  // Returns nullptr if there is no current behavior
  const IBehavior* GetCurrentBehavior() const { return _currentBehavior; }

  const IBehaviorChooser* GetBehaviorChooser() const { return _behaviorChooser; }
    
  const BehaviorFactory& GetBehaviorFactory() const { assert(_behaviorFactory); return *_behaviorFactory; }
        BehaviorFactory& GetBehaviorFactory()       { assert(_behaviorFactory); return *_behaviorFactory; }

  // accessors: whiteboard
  const BehaviorWhiteboard& GetWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
        BehaviorWhiteboard& GetWhiteboard()       { assert(_whiteboard); return *_whiteboard; }
    
  //
  IBehavior* LoadBehaviorFromJson(const Json::Value& behaviorJson);
    
  void ClearAllBehaviorOverrides();
  bool OverrideBehaviorScore(const std::string& behaviorName, float newScore);
    
  void HandleMessage(const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message);

private:
    
  bool _isInitialized;
  bool _forceReInit;
    
  Robot& _robot;
    
  void   SwitchToNextBehavior();
  Result InitNextBehaviorHelper();
  void   AddReactionaryBehavior(IReactionaryBehavior* behavior);

  void   StopCurrentBehavior();
  void   SetCurrentBehavior(IBehavior* newBehavior);
    
  // Factory creates and tracks data-driven behaviors etc
  BehaviorFactory* _behaviorFactory;
    
  // How we store and choose next behavior
  IBehaviorChooser* _behaviorChooser = nullptr;
  // The default chooser is created once on startup
  IBehaviorChooser* _defaultChooser = nullptr;
    
  IBehavior* _currentBehavior = nullptr;
  IBehavior* _nextBehavior = nullptr;
  IBehavior* _forceSwitchBehavior = nullptr;
    
  // whiteboard for behaviors to share information, or to store information only useful to behaviors
  std::unique_ptr<BehaviorWhiteboard> _whiteboard;
    
  // Minimum amount of time to stay in each behavior
  float _minBehaviorTime_sec;
  float _lastSwitchTime_sec;
    
  // For random numbers
  Util::RandomGenerator _rng;
    
  // For storing event handlers
  std::vector<Signal::SmartHandle> _eventHandlers;
    
}; // class BehaviorManager

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_MANAGER_H
