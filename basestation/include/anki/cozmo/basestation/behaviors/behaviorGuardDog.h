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

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
// forward declarations:
class BlockWorldFilter;
  
class BehaviorGuardDog : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorGuardDog(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}

protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  
  // Message handlers:
  void HandleObjectAccel(Robot& robot, const ObjectAccel& msg);
  void HandleObjectConnectionState(const Robot& robot, const ObjectConnectionState& msg);
  void HandleObjectMoved(const Robot& robot, const ObjectMoved& msg);
  void HandleObjectUpAxisChanged(Robot& robot, const ObjectUpAxisChanged& msg);
  
  // Helpers:
  
  // Compute a goal starting pose near the blocks
  void ComputeStartingPose(const Robot& robot, Pose3d& startingPose);
  
  // Send message to robot to start/stop streaming ObjectAccel messages from all cubes
  Result EnableCubeAccelStreaming(const Robot& robot, const bool enable = true) const;
  
  // Start/stop monitoring for cube motion (calls EnableCubeAccelStreaming)
  void StartMonitoringCubeMotion(Robot& robot, const bool enable = true);
  
  // Start a light cube animation on the specified cube
  bool StartLightCubeAnim(Robot& robot, const ObjectID& objId, const CubeAnimationTrigger& cubeAnimTrigger);
  
  // Start a light cube animation on all cubes
  bool StartLightCubeAnims(Robot& robot, const CubeAnimationTrigger& cubeAnimTrigger);
  
private:
  
  enum class State {
    Init,                   // Initialize everything and play starting animations
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
    float accelMag          = 0.f;   // latest accelerometer magnitude
    float prevAccelMag      = 0.f;   // previous accelerometer magnitude
    float hpFiltAccelMag    = 0.f;   // high-pass filtered accel magnitude
    float maxFiltAccelMag   = 0.f;   // maximum filtered accel magnitude encountered
    float movementScore     = 0.f;   // keeps track of how much the block is being moved (decays if block is not moving, capped at kMovementScoreMax)
    bool filtInitialized    = false; // high-pass filter initialized?
    bool hasBeenMoved       = false; // has the block been moved at all?
    bool hasBeenFlipped     = false; // has the block been successfully flipped?
    uint msgReceivedCnt     = 0;     // how many ObjectAccel messages have we received for this block?
    uint badMsgCnt          = 0;     // how many weird ObjectAccel e.g. accel fields blank or really large) message have we received?
    CubeAnimationTrigger lastCubeAnimTrigger = CubeAnimationTrigger::Count;  // the last-played animation trigger for this cube.
    UpAxis upAxis           = UpAxis::ZPositive;  // The last reported UpAxis for the cube
    
    // member functions:
    void ResetAccelData() {
      accelMag        = 0.f;
      prevAccelMag    = 0.f;
      hpFiltAccelMag  = 0.f;
      maxFiltAccelMag = 0.f;
      movementScore   = 0.f;
      filtInitialized = false;
    }
  };
  
  // Stores relevant data for each of the cubes
  std::map<ObjectID, sCubeData> _cubesDataMap;
  
  // Blockworld filter that gets used often (set up in constructor)
  std::unique_ptr<BlockWorldFilter> _connectedCubesOnlyFilter;
  
  // Time that Cozmo first fell asleep (used for overall timeout detection)
  float _firstSleepingStartTime_s = 0.f;
  
  // Time of the last cube movement (used for timeout detection)
  float _lastCubeMovementTime_s = 0.f;
  
  // true when monitoring for cube movement
  bool _monitoringCubeMotion = false;
  
  // The number of cubes that have been moved during the behavior
  int _nCubesMoved = 0;
  
  // The number of cubes that have been successfully flipped over
  int _nCubesFlipped = 0;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorGuardDog_H__
