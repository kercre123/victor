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
#include "anki/common/basestation/objectIDs.h"

#include "clad/types/behaviorTypes.h"
#include "clad/types/unlockTypes.h"


#include "util/helpers/templateHelpers.h"
#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"

#include "json/json-forwards.h"

#include <assert.h>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <float.h>


namespace Anki {
namespace Cozmo {
  
// Forward declaration
class BehaviorFactory;
class AIWhiteboard;
class AIGoalEvaluator;
class IBehavior;
class IBehaviorChooser;
class IReactionaryBehavior;
class Robot;
class WorkoutComponent;

template<typename TYPE> class AnkiEvent;

namespace Audio {
class BehaviorAudioClient;
}
namespace ExternalInterface {
class BehaviorManagerMessageUnion;
}

static const f32 kIgnoreDefaultHeandAndLiftState = FLT_MAX;
  
class BehaviorManager
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  BehaviorManager(Robot& robot);
  ~BehaviorManager();

  // initialize this behavior manager from the given Json config
  Result InitConfiguration(const Json::Value& config);
  
  // create a behavior from the given config and add to the factory. Can fail if the configuration is not valid
  Result CreateBehaviorFromConfiguration(const Json::Value& behaviorConfig);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Games
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // get if the given flags are available, you can check for all or if any is set
  inline bool AreAllGameFlagsAvailable(BehaviorGameFlag gameFlag) const;
  inline bool IsAnyGameFlagAvailable(BehaviorGameFlag gameFlag) const;
  UnlockId GetActiveSpark() const { return _activeSpark; };
  UnlockId GetRequestedSpark() const { return _lastRequestedSpark; };
  bool IsActiveSparkSoft() const  { return (_activeSpark != UnlockId::Count) && _isSoftSpark;}
  bool IsRequestedSparkSoft() const  { return (_lastRequestedSpark != UnlockId::Count) && _isRequestedSparkSoft;}

  // sets which games are available by setting the mask/flag combination
  void SetAvailableGame(BehaviorGameFlag availableGames) { _availableGames = Util::EnumToUnderlying(availableGames); }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Calls the current behavior's Update() method until it returns COMPLETE or FAILURE.
  Result Update(Robot& robot);
        
  // Set the current IBehaviorChooser. Note this results in the destruction of the current IBehaviorChooser
  // (if there is one) and the IBehaviors it contains
  void SetBehaviorChooser(IBehaviorChooser* newChooser);
  
  // returns last time we changerd behavior chooser
  float GetLastBehaviorChooserSwitchTime() const { return _lastChooserSwitchTime; }

  // Stops the current behavior and switches to null. The next time Update is called, the behavior chooser
  // will have a chance to select a new behavior. This is mostly useful as a hack to force a behavior switch
  // when it is needed (e.g. sparks). `stoppedByWhom` is a string for debugging so we know why this was stopped
  void RequestCurrentBehaviorEndImmediately(const std::string& stoppedByWhom);
  
  // Returns nullptr if there is no current behavior
  const IBehavior* GetCurrentBehavior() const { return _currentBehavior; }

  const IBehaviorChooser* GetBehaviorChooser() const { return _currentChooserPtr; }
    
  const BehaviorFactory& GetBehaviorFactory() const { assert(_behaviorFactory); return *_behaviorFactory; }
        BehaviorFactory& GetBehaviorFactory()       { assert(_behaviorFactory); return *_behaviorFactory; }
  
  // Enable and disable reactionary behaviors
  void RequestEnableReactionaryBehavior(const std::string& requesterID, BehaviorType behavior, bool enable);
  
  // accessors: whiteboard
  const AIWhiteboard& GetWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
        AIWhiteboard& GetWhiteboard()       { assert(_whiteboard); return *_whiteboard; }
  
  const WorkoutComponent& GetWorkoutComponent() const { assert(_workoutComponent); return *_workoutComponent; }
        WorkoutComponent& GetWorkoutComponent()       { assert(_workoutComponent); return *_workoutComponent; }

  // accessors: audioController
  Audio::BehaviorAudioClient& GetAudioClient() const { assert(_audioClient); return *_audioClient;}
  
  void HandleMessage(const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message);
  
  void SetReactionaryBehaviorsEnabled(bool isEnabled, bool stopCurrent = false);
  void RequestCurrentBehaviorEndOnNextActionComplete();
  
  // Returns first reactionary behavior with given type or nullptr if none found
  IReactionaryBehavior* GetReactionaryBehaviorByType(BehaviorType behaviorType);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Freeplay - specific
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // calculates (and sets in the freeplay chooser) the desired goal due to objects recently seen
  void CalculateFreeplayGoalFromObjects();

  // return the basestation time that freeplay first started (often useful as a notion of "session"). This
  // will be -1 until freeplay has started, and then will always be set to the time (in seconds) that freeplay
  // started
  float GetFirstTimeFreeplayStarted() const { return _firstTimeFreeplayStarted; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Sparks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Allows Goal evaluator to know if it should kick out the current goal and transition into a spark
  // Goal evaluator only switches from non-sparked to sparked.  Otherwise the sparkBehaviorChooser will kill itself
  bool ShouldSwitchToSpark() const { return _activeSpark == UnlockId::Count && _activeSpark != _lastRequestedSpark;}
  
  // Actually switches out the LastRequestedSpark from the game with the active spark
  // Returns the UnlockID of the newly active spark
  const UnlockId SwitchToRequestedSpark();
  
  // Allows sparks to know if the game has requested that they quit when able
  bool DidGameRequestSparkEnd() const{ return _didGameRequestSparkEnd;}
  
  // Stores the requested spark until it (or a subsequent request) is swapped in to ActiveSpark with a call to
  // SwitchToRequestedSpark.  This simplifies asyncronous messaging issues
  // and gives sparks the opportunity to end themselves if they are canceled by the user
  void SetRequestedSpark(UnlockId spark, bool softSpark);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ObjectTapInteractions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Handles switching behavior chooser, goal, and updating whiteboard for object tap interactions
  void HandleObjectTapInteraction(const ObjectID& objectID);
  
  // Leave object interaction state resets the goal and clears the tap intented object in the whiteboard
  void LeaveObjectTapInteraction();
  
  const ObjectID& GetLastTappedObject() const { return _lastDoubleTappedObject; }
  const ObjectID& GetCurrTappedObject() const { return _currDoubleTappedObject; }
  
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // switches to a given behavior (stopping the current behavior if necessary). Returns true if it switched
  bool SwitchToBehavior(IBehavior* nextBehavior);

  // same as SwitchToBehavior but also handles special reactionary logic
  void SwitchToReactionaryBehavior(IReactionaryBehavior* nextBehavior);
  
  // checks the chooser and switches to a new behavior if neccesary
  void ChooseNextBehaviorAndSwitch();
  
  // try to resume back to the previous behavior (after interruptions). This method does not guarantee resuming
  // to a previous one. If there was no behavior to resume to, it will set null behavior as current.
  void TryToResumeBehavior();

  // stop the current behavior if it is non-null and running (i.e. Init was called)
  void StopCurrentBehavior();
  
  //Allow reactionary behaviors to request a switch without a message
  void CheckForComputationalSwitch();

  void SendDasTransitionMessage(IBehavior* oldBehavior, IBehavior* newBehavior);
  
  void AddReactionaryBehavior(IReactionaryBehavior* behavior);
  
  // Allow unity to set head angles and lift heights to preserve after reactionary behaviors
  void SetDefaultHeadAndLiftState(bool enable, f32 headAngle, f32 liftHeight);
  bool AreDefaultHeandAndLiftStateSet() { return _defaultHeadAngle != kIgnoreDefaultHeandAndLiftState;}

  // check if there is a matching reactionary behavior which wants to run for the given event, and switch to
  // it if so
  template<typename EventType>
  void ConsiderReactionaryBehaviorForEvent(const AnkiEvent<EventType>& event);
  
  // update the tapped object should its pose change
  void UpdateTappedObject();
  
  // update current behavior with the new tapped object
  void UpdateBehaviorWithObjectTapInteraction();

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  bool _isInitialized = false;
    
  Robot& _robot;
  
  // Set by unity to preserve the heand and lift angle after reactionary behaviors
  f32 _defaultHeadAngle;
  f32 _defaultLiftHeight;
  
  // - - - - - - - - - - - - - - -
  // current running behavior
  // - - - - - - - - - - - - - - -
  
  bool _runningReactionaryBehavior = false;
  
  // behavior currently running and responsible for controlling the robot
  IBehavior* _currentBehavior  = nullptr;
  
  // this is the behavior to go back to after a reactionary behavior is completed
  IBehavior* _behaviorToResume = nullptr;

  // Once we are done with reactionary behaviors, should we try to go back to _behaviorToResume? If any
  // reactionary behavior we run says "no", this will get set to false, otherwise it will stay true
  bool _shouldResumeBehaviorAfterReaction = true;
  
  // current behavior chooser (weak_ptr pointing to one of the others)
  IBehaviorChooser* _currentChooserPtr = nullptr;
  
  // - - - - - - - - - - - - - - -
  // factory & choosers
  // - - - - - - - - - - - - - - -
  
  // Factory creates and tracks data-driven behaviors etc
  BehaviorFactory* _behaviorFactory = nullptr;
    
  // This is a chooser which manually runs specific behaviors. The manager starts out using this chooser to
  // avoid immediately executing a behavior when the engine starts
  IBehaviorChooser* _selectionChooser = nullptr;

  // Behavior chooser for freeplay mode, it also includes games and sparks if needed
  IBehaviorChooser* _freeplayChooser = nullptr;
  
  // behavior chooser for meet cozmo activity
  IBehaviorChooser* _meetCozmoChooser = nullptr;
  
  // list of behaviors that fire automatically as reactions to events
  std::vector<IReactionaryBehavior*> _reactionaryBehaviors;
  bool                               _reactionsEnabled = true;
  
  // time at which last chooser was selected
  float _lastChooserSwitchTime;

  // first time freeplay ever started
  float _firstTimeFreeplayStarted = -1.0f;

  // - - - - - - - - - - - - - - -
  // others/shared
  // - - - - - - - - - - - - - - -
  
  // games that are available currently for Cozmo to request
  using BehaviorGameFlagMask = std::underlying_type<BehaviorGameFlag>::type;
  BehaviorGameFlagMask _availableGames = Util::EnumToUnderlying( BehaviorGameFlag::NoGame );
  
  // current active spark (this does guarantee that behaviors will kick in, only that Cozmo is in a Sparked state)
  UnlockId _activeSpark = UnlockId::Count;
  // Identifies if the spark is a "soft spark" - played when upgrade is unlocked and has some different playback features than normal sparks
  bool _isSoftSpark = false;
  
  // holding variables to keep track of most recent spark messages from game until goalEvaluator Switches out the active spark
  UnlockId _lastRequestedSpark = UnlockId::Count;
  bool _isRequestedSparkSoft = false;
  bool _didGameRequestSparkEnd = false;
  
  // Behavior audio client is used to update the audio engine with the current sparked state (a.k.a. "round")
  std::unique_ptr<Audio::BehaviorAudioClient> _audioClient;
  
  // whiteboard for behaviors to share information, or to store information only useful to behaviors
  std::unique_ptr<AIWhiteboard> _whiteboard;

  // component for tracking cozmo's work-outs
  std::unique_ptr< WorkoutComponent > _workoutComponent;
    
  // For storing event handlers
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  // The last object that was double tapped used for clearing object tap interaction lights
  ObjectID _lastDoubleTappedObject;
  ObjectID _currDoubleTappedObject;
  
  // The object that was just double tapped, will become current double tapped object on next Update()
  ObjectID _pendingDoubleTappedObject;
  
  // Whether or not we need to handle an object being tapped in Update()
  bool _needToHandleObjectTapped = false;
    
}; // class BehaviorManager

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Inline
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::AreAllGameFlagsAvailable(BehaviorGameFlag gameFlag) const
{
  const BehaviorGameFlagMask flags = Util::EnumToUnderlying(gameFlag);
  const bool allSet = ((_availableGames & flags) == flags);
  return allSet;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::IsAnyGameFlagAvailable(BehaviorGameFlag gameFlag) const
{
  const BehaviorGameFlagMask flags = Util::EnumToUnderlying(gameFlag);
  const bool anySet = ((_availableGames & flags) != 0);
  return anySet;
}

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_MANAGER_H
