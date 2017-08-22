/**
* File: behaviorSystemManager.h
*
* Author: Kevin Karol
* Date:   8/17/2017
*
* Description: Manages and enforces the lifecycle and transitions
* of partso of the behavior system
*
* Copyright: Anki, Inc. 2017
**/

#ifndef COZMO_BEHAVIOR_SYSTEM_MANAGER_H
#define COZMO_BEHAVIOR_SYSTEM_MANAGER_H

#include "anki/common/types.h"
#include "anki/common/basestation/objectIDs.h"
#include "engine/behaviorSystem/behaviors/iBehavior_fwd.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "json/json-forwards.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

// Forward declarations
class BehaviorContainer;
class IActivity;
class Robot;

struct BehaviorRunningInfo;
  
namespace Audio {
class BehaviorAudioClient;
}

class BehaviorSystemManager
{
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  BehaviorSystemManager(Robot& robot);
  ~BehaviorSystemManager();

  // initialize this behavior manager from the given Json config
  Result InitConfiguration(Robot& robot, const Json::Value& behaviorSystemConfig);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Calls the current behavior's Update() method until it returns COMPLETE or FAILURE.
  Result Update(Robot& robot);
  
  // returns nullptr if there is no current behavior
  const IBehaviorPtr GetCurrentBehavior() const;

  IBehaviorPtr FindBehaviorByID(BehaviorID behaviorID) const;
  IBehaviorPtr FindBehaviorByExecutableType(ExecutableBehaviorType type) const;

  // Sometimes it's necessary to downcast to a behavior to a specific behavior pointer, e.g. so an Activity
  // can access it's member functions. This function will help with that and provide a few assert checks along
  // the way. It sets outPtr in arguemnts, and returns true if the cast is successful
  template<typename T>
  bool FindBehaviorByIDAndDowncast(BehaviorID behaviorID,
                                   BehaviorClass requiredClass, 
                                   std::shared_ptr<T>& outPtr ) const;
  // TODO:(bn) automatically infer requiredClass from T

  // accessors: audioController
  Audio::BehaviorAudioClient& GetAudioClient() const { assert(_audioClient); return *_audioClient;}
  
protected:
  // Allow tests to access the behavior container directly so that they
  // can create test behaviors
  BehaviorContainer& GetBehaviorContainer(){ assert(_behaviorContainer); return *_behaviorContainer;}
  
private:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InitializeEventHandlers(Robot& robot);
  
  bool SwitchToBehaviorBase(Robot& robot, BehaviorRunningInfo& nextBehaviorInfo);
  
  // checks the chooser and switches to a new behavior if neccesary
  void UpdateActiveBehavior(Robot& robot);
  
  // stop the current behavior if it is non-null and running (i.e. Init was called)
  void StopAndNullifyCurrentBehavior();
  
  // Called at the Complete or Failed state of a behavior in order to switch to a new one
  void FinishCurrentBehavior(Robot& robot, IBehaviorPtr activeBehavior);

  void SendDasTransitionMessage(Robot& robot,
                                const BehaviorRunningInfo& oldBehaviorInfo,
                                const BehaviorRunningInfo& newBehaviorInfo);
  
  // Functions which mediate direct access to running/resume info so that the robot
  // can respond appropriately to switching between reactions/resumes
  BehaviorRunningInfo& GetRunningInfo() const {assert(_runningInfo); return *_runningInfo;}
  void SetRunningInfo(Robot& robot, const BehaviorRunningInfo& newInfo);

  // helper to avoid including iBehavior.h here
  BehaviorClass GetBehaviorClass(IBehaviorPtr behavior) const;
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  bool _isInitialized;
  
  // - - - - - - - - - - - - - - -
  // current running behavior
  // - - - - - - - - - - - - - - -
  
  // PLEASE DO NOT ACCESS OR ASSIGN TO DIRECTLY - use functions above
  std::unique_ptr<BehaviorRunningInfo> _runningInfo;
  
  // - - - - - - - - - - - - - - -
  // factory & choosers
  // - - - - - - - - - - - - - - -
  
  // Factory creates and tracks data-driven behaviors etc
  std::unique_ptr<BehaviorContainer> _behaviorContainer;

  // - - - - - - - - - - - - - - -
  // others/shared
  // - - - - - - - - - - - - - - -
  
  std::unique_ptr<IActivity> _baseActivity;
  
  // Behavior audio client is used to update the audio engine with the current sparked state (a.k.a. "round")
  std::unique_ptr<Audio::BehaviorAudioClient> _audioClient;
      
}; // class BehaviorSystemManager

template<typename T>
bool BehaviorSystemManager::FindBehaviorByIDAndDowncast(BehaviorID behaviorID,
                                                        BehaviorClass requiredClass,
                                                        std::shared_ptr<T>& outPtr) const
{

  IBehaviorPtr behavior = FindBehaviorByID(behaviorID);
  if( ANKI_VERIFY(behavior != nullptr,
                  "BehaviorContainer.FindBehaviorByIDAndDowncast.NoBehavior",
                  "BehaviorID: %s requiredClass: %s",
                  BehaviorIDToString(behaviorID),
                  BehaviorClassToString(requiredClass)) &&

      ANKI_VERIFY(behavior != nullptr && GetBehaviorClass(behavior) == requiredClass,
                  "BehaviorContainer.FindBehaviorByIDAndDowncast.WrongClass",
                  "BehaviorID: %s requiredClass: %s",
                  BehaviorIDToString(behaviorID),
                  BehaviorClassToString(requiredClass)) ) {

    outPtr = std::static_pointer_cast<T>(behavior);

    if( ANKI_VERIFY(outPtr != nullptr, "BehaviorContainer.FindBehaviorByIDAndDowncast.CastFailed",
                    "BehaviorID: %s requiredClass: %s",
                    BehaviorIDToString(behaviorID),
                    BehaviorClassToString(requiredClass)) ) {
      return true;
    }
  }

  return false;
}

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_SYSTEM_MANAGER_H
