/**
 * File: BehaviorAudioComponent.h
 *
 * Author: Jordan Rivas
 * Created: 11/02/2016
 *
 * Description: This Client provides behavior audio needs for updating Sparked Behavior music state and round. When
 *              using first call ActivateSparkedMusic() to activate audio client and when behavior is completed call
 *              DeactivateSparkedMusic().
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

// TODO: VIC-25 - Migrate Behavior Audio Client to victor
// This class controlls behavior music state which is not realevent when playing audio on the robot

#ifndef __Basestation_Audio_BehaviorAudioComponent_H__
#define __Basestation_Audio_BehaviorAudioComponent_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/events/ankiEventMgr.h"

#include "clad/audio/audioStateTypes.h"
#include "clad/audio/audioSwitchTypes.h"
#include "clad/types/robotPublicState.h"


#define kBehaviorRound  0

namespace Anki {
namespace Vector {

class BehaviorExternalInterface;
class BehaviorManager;

namespace Audio {

class EngineRobotAudioClient;

class BehaviorAudioComponent : public IDependencyManagedComponent<BCComponentID> {
public:
  BehaviorAudioComponent(Audio::EngineRobotAudioClient* robotAudioClient);
  
  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Vector::Robot* robot, const BCCompMap& dependentComps) override;
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override { 
    dependencies.insert(BCComponentID::BehaviorExternalInterface);
  };
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////

  void Init(BehaviorExternalInterface& behaviorExternalInterface);

  // True if Client has been Activated
  bool IsActive() const { return _isActive; }
  
  // Get Current round
  int GetRound() const { return _round; }
  
  // Change music switch state for Ai Goals in freeplay
  void UpdateActivityMusicState(const BehaviorID& activityID);
  
protected:
  
  void HandleRobotPublicStateChange(BehaviorExternalInterface& behaviorExternalInterface,
                                    const RobotPublicState& stateEvent);
  
private:
  // Track second unlockID value for instances where we receive the appropriate
  // music state from game after a spark activity has already started
  //UnlockId  _lastUnlockIDReceived = UnlockId::Count;
  
  bool      _isActive = false;
  int       _round    = kBehaviorRound;
  
// World Event tracking
//  bool      _isCubeInLift     = false;
//  bool      _isRequestingGame = false;
//  bool      _stackExists      = false;
  
  std::vector<::Signal::SmartHandle> _eventHandles;
  
  void SetDefaultBehaviorRound() { _round = kBehaviorRound; }
  
  // Keep track of the last AI goal name sent from the PublicStateBroadcaster so we
  //  can properly handle AI goal transitions.
  //BehaviorID _prevActivity;
  
  // Tracks the active behavior stage if custom music rounds are being set
  // Use setter/getter function rather than accessing directly
  BehaviorStageTag _activeBehaviorStage;
  
  // Keep track of the last GuardDog behavior stage so we can set the
  //  proper audio round when transitioning between stages.
//  GuardDogStage _prevGuardDogStage;
  
  // Keep track of the last feeding stage so we can identify music changes
//  FeedingStage _prevFeedingStage;
  
  // Communicate with the audio client
  //Audio::RobotAudioClient* _robotAudioClient;
  
  // Keep track of the last needs levels so we can broadcast when they change
//  NeedsLevels _needsLevel;
  
  
  BehaviorStageTag GetActiveBehaviorStage();
  void SetActiveBehaviorStage(BehaviorStageTag stageTag);
  
  void HandleWorldEventUpdates(const RobotPublicState& stateEvent);
  void HandleGuardDogUpdates(const BehaviorStageStruct& currPublicStateStruct);
  void HandleDancingUpdates(const BehaviorStageStruct& currPublicStateStruct);
  void HandleFeedingUpdates(const BehaviorStageStruct& currPublicStateStruct);
  void HandleNeedsUpdates(const NeedsLevels& needsLevel);
  void HandleDimMusicForActivity(const RobotPublicState& stateEvent);

  
};

} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_BehaviorAudioComponent_H__ */
