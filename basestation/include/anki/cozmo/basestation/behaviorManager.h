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

#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"

#include "json/json-forwards.h"

#include <assert.h>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace Anki {
namespace Cozmo {
  
// Forward declaration
class BehaviorFactory;
class AIWhiteboard;
class IBehavior;
class IBehaviorChooser;
class IReactionaryBehavior;
class Robot;
template<typename TYPE> class AnkiEvent;

  
namespace ExternalInterface {
class BehaviorManagerMessageUnion;
}

class BehaviorManager
{
public:
    
  BehaviorManager(Robot& robot);
  ~BehaviorManager();
    
  Result Init(const Json::Value& config);
    
  // Calls the current behavior's Update() method until it returns COMPLETE or FAILURE.
  Result Update();
        
  // Forcefully select the next behavior by name (versus by letting the selection mechanism choose based on
  // current state). Fails if that behavior does not exist or the selected behavior is not runnable.
  Result SelectBehavior(const std::string& name);
  
  // Set the current IBehaviorChooser. Note this results in the destruction of the current IBehaviorChooser
  // (if there is one) and the IBehaviors it contains
  void SetBehaviorChooser(IBehaviorChooser* newChooser);
    
  // Returns nullptr if there is no current behavior
  const IBehavior* GetCurrentBehavior() const { return _currentBehavior; }

  const IBehaviorChooser* GetBehaviorChooser() const { return _behaviorChooser; }
    
  const BehaviorFactory& GetBehaviorFactory() const { assert(_behaviorFactory); return *_behaviorFactory; }
        BehaviorFactory& GetBehaviorFactory()       { assert(_behaviorFactory); return *_behaviorFactory; }

  // accessors: whiteboard
  const AIWhiteboard& GetWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
        AIWhiteboard& GetWhiteboard()       { assert(_whiteboard); return *_whiteboard; }
    
  IBehavior* LoadBehaviorFromJson(const Json::Value& behaviorJson);
    
  void ClearAllBehaviorOverrides();
  bool OverrideBehaviorScore(const std::string& behaviorName, float newScore);
    
  void HandleMessage(const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message);

private:

  // switches to a given behavior (stopping the current behavior if necessary). Returns true if it switched
  bool SwitchToBehavior(IBehavior* nextBehavior);

  // same as SwitchToBehavior but also handles special reactionary logic
  void SwitchToReactionaryBehavior(IBehavior* nextBehavior);

  // checks the chooser and switches to a new behavior if neccesary
  void SwitchToNextBehavior();

  // If there is a behavior to resume, switch to it and return true. Otherwise return false
  bool ResumeBehaviorIfNeeded();

  // stop the current behavior if it is non-null and running (i.e. Init was called)
  void StopCurrentBehavior();

  static void SendDasTransitionMessage(IBehavior* oldBehavior, IBehavior* newBehavior);
  
  void AddReactionaryBehavior(IReactionaryBehavior* behavior);

  // check if there is a matching reactionary behavior which wants to run for the given event, and switch to
  // it if so
  template<typename EventType>
  void ConsiderReactionaryBehaviorForEvent(const AnkiEvent<EventType>& event);

  bool _isInitialized;
    
  Robot& _robot;    
    
  // Factory creates and tracks data-driven behaviors etc
  BehaviorFactory* _behaviorFactory;
    
  // How we store and choose next behavior
  IBehaviorChooser* _behaviorChooser = nullptr;

  // This is a chooser which manually runs specific behaviors. The manager starts out using this chooser to
  // avoid immediately executing a behavior when the engine starts
  IBehaviorChooser* _selectionChooser = nullptr;

  // This is the behavior used for freeplay. This is the default chooser
  IBehaviorChooser* _freeplayBehaviorChooser = nullptr;

  bool _runningReactionaryBehavior = false;
  
  IBehavior* _currentBehavior = nullptr;

  // This is the behavior to go back to after a reactionary behavior is completed
  IBehavior* _behaviorToResume = nullptr;
    
  // whiteboard for behaviors to share information, or to store information only useful to behaviors
  std::unique_ptr<AIWhiteboard> _whiteboard;

  std::vector<IReactionaryBehavior*> _reactionaryBehaviors;
    
  // For random numbers
  Util::RandomGenerator _rng;
    
  // For storing event handlers
  std::vector<Signal::SmartHandle> _eventHandlers;
    
}; // class BehaviorManager

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_MANAGER_H
