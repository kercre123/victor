/**
 * File: behaviorGuardDog.h
 *
 * Author: Matt Michini
 * Created: 2017-03-27
 *
 * Description: Cozmo acts as a sleepy guard dog, pretending to nap but still making sure no one tries to steal his cubes.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorGuardDog_H__
#define __Cozmo_Basestation_Behaviors_BehaviorGuardDog_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/behaviorComponent/behaviorStages.h"

namespace Anki {
namespace Cozmo {
  
// forward declarations:
class BlockWorldFilter;
namespace CubeAccelListeners {
  class MovementListener;
}
  
class BehaviorGuardDog : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorGuardDog(const Json::Value& config);
  
public:  
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }


  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  
private:
  
  // Message handlers:
  void HandleObjectConnectionState(const ObjectConnectionState& msg);
  void HandleObjectMoved(const ObjectMoved& msg);
  void HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg);
  void CubeMovementHandler(const ObjectID& objId, const float movementScore);

  // Helpers:
  
  // Compute a goal starting pose near the blocks
  void ComputeStartingPose(Pose3d& startingPose);
  
  // Start/stop monitoring for cube motion
  void StartMonitoringCubeMotion(const bool enable = true);
  
  // Start a light cube animation on the specified cube
  bool StartLightCubeAnim(const ObjectID& objId, const CubeAnimationTrigger& cubeAnimTrigger);
  
  // Start a light cube animation on all cubes
  bool StartLightCubeAnims(const CubeAnimationTrigger& cubeAnimTrigger);
  
  // Update the behavior stage for the PublicStateBroadcaster (this is used to trigger the proper
  //   music to match the current behavior state, e.g. tension for when a cube is being moved)
  void UpdatePublicBehaviorStage(const GuardDogStage& stage);
  
  // Record the game result (also computes the game duration)
  void RecordResult(std::string&& result);
  
  // Log out some analytics events at the end of the behavior
  void LogDasEvents() const;
  
private:
  
  enum class State {
    Init,                   // Initialize everything and play starting animations
    SetupInterrupted,       // The game was interrupted before Cozmo fell asleep for the first time (e.g. blocks moved)
    DriveToBlocks,          // Drive to a pose near the blocks
    SettleIn,               // Play animation for 'settling in' before sleeping
    StartSleeping,
    Sleeping,
    Fakeout,                // "half wakeup" due to one or two cubes being moved
    Busted,                 // Freak out because all 3 cubes have been touched/moved
    BlockDisconnected,      // A block has disconnected (play an anim and end the behavior)
    Timeout,                // Timeout expired and player hasn't flipped all cubes or disturbed Cozmo.
    PlayerSuccess,          // Player successfully flipped all three cubes
    Complete
  };
  
  State _state = State::Init;
  
  // struct to hold data about the blocks in question
  struct sCubeData {
    float movementScore     = 0.f;   // keeps track of how much the block is being moved (decays if block is not moving, capped at kMovementScoreMax)
    float maxMovementScore  = 0.f;   // the maximum experienced movement score for this cube
    bool hasBeenMoved       = false; // has the block been moved at all?
    bool hasBeenFlipped     = false; // has the block been successfully flipped?
    float firstMovedTime_s  = 0.f;   // first time that the block was moved (in basestation timer seconds)
    float flippedTime_s     = 0.f;   // time that the block was successfully flipped by the player (in basestation timer seconds)
    CubeAnimationTrigger lastCubeAnimTrigger = CubeAnimationTrigger::Count;  // the last-played animation trigger for this cube.
    UpAxis upAxis           = UpAxis::ZPositive;  // The last reported UpAxis for the cube
    
    std::shared_ptr<CubeAccelListeners::MovementListener> cubeMovementListener; // CubeAccelComponent listener
  };
  
  // Stores relevant data for each of the cubes
  std::map<ObjectID, sCubeData> _cubesDataMap;
  
  // Blockworld filter that gets used often (set up in constructor)
  std::unique_ptr<BlockWorldFilter> _connectedCubesOnlyFilter;
  
  // Time that Cozmo first fell asleep (used for overall timeout detection)
  float _firstSleepingStartTime_s = 0.f;
  
  // Total duration that Cozmo was asleep
  float _sleepingDuration_s = 0.f;
  
  // Time of the last cube movement (used for timeout detection)
  float _lastCubeMovementTime_s = 0.f;
  
  // true when monitoring for cube movement
  bool _monitoringCubeMotion = false;
  
  // The number of cubes that have been moved during the behavior
  int _nCubesMoved = 0;
  
  // The number of cubes that have been successfully flipped over
  int _nCubesFlipped = 0;
  
  // The current 'PublicStateBroadcaster' behavior stage (used to trigger
  //   the proper music depending on the current state of the behavior)
  GuardDogStage _currPublicBehaviorStage = GuardDogStage::Count;
  
  // The result of the GuardDog 'game' (e.g. "PlayerSuccess" or "Busted")
  std::string _result;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorGuardDog_H__
