/**
 * File: behviorGameRequest.h
 *
 * Author: Brad Neuman
 * Created: 2016-02-02
 *
 * Description: This is a base class for all behaviors which request mini game activities
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_GameRequest_BehaviorGameRequest_H__
#define __Cozmo_Basestation_Behaviors_GameRequest_BehaviorGameRequest_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "coretech/vision/engine/trackedFace.h"
#include <memory>
#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class DockingComponent;
class ObservableObject;

namespace ExternalInterface {
struct RobotDeletedFace;
struct RobotObservedFace;
}

class ICozmoBehaviorRequestGame : public ICozmoBehavior
{
public:

  ICozmoBehaviorRequestGame(const Json::Value& config);

  // final to ensure subclass does not skip. If you need to override in subclass I suggest another internal one
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const final override;
  
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) final override;
  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) final override;
  virtual bool ShouldCancelWhenInControl() const override { return false;}

protected:

  using Face = Vision::TrackedFace;
  using FaceID_t = Vision::FaceID_t;
  
  void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // --------------------------------------------------------------------------------
  // Functions to be overridden by subclasses
  
  virtual void RequestGame_OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  virtual void RequestGame_UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  virtual void HandleGameDeniedRequest(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  virtual void RequestGame_OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) { }

  // --------------------------------------------------------------------------------
  // Utility functions for subclasses
  
  // these send the message (don't do anything with animations)
  void SendRequest(BehaviorExternalInterface& behaviorExternalInterface);
  void SendDeny(BehaviorExternalInterface& behaviorExternalInterface);

  // the time at which it will be OK to end the behavior (allowing us a delay after the request), or -1
  virtual f32 GetRequestMinDelayComplete_s() const = 0;

  FaceID_t GetFaceID() const { return _faceID; }
  f32 GetLastSeenFaceTime() const {return _lastFaceSeenTime_s;}
    
  virtual u32 GetNumBlocks(BehaviorExternalInterface& behaviorExternalInterface) const;

  // The first call to this will set the block ID, then it will be consistent until the behavior is reset
  ObjectID GetRobotsBlockID(BehaviorExternalInterface& behaviorExternalInterface);

  // If possible, this call will switch the robot to a different block ID. It will add the current ID to a
  // blacklist (which gets reset next time this behavior runs). Returns true if successful
  bool SwitchRobotsBlock(BehaviorExternalInterface& behaviorExternalInterface);

  // Gets the last known pose of the block (useful in case it gets bumped). returns true if it has a block
  // pose, false otherwise
  bool GetLastBlockPose(Pose3d& pose) const;

  bool HasFace(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  // Returns true if HasFace() and returns the pose as argument. Returns false and leaves argument alone otherwise.
  bool GetFacePose(BehaviorExternalInterface& behaviorExternalInterface, Pose3d& facePoseWrtRobotOrigin) const;
  
  virtual void HandleCliffEvent(BehaviorExternalInterface& behaviorExternalInterface, const EngineToGameEvent& event) {};

  // --------------------------------------------------------------------------------
  // Functions from ICozmoBehavior which aren't exposed to children

  virtual void AlwaysHandleInScope(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) final override;
  virtual void AlwaysHandleInScope(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) final override;
  virtual void HandleWhileActivated(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) final override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) final override;
  
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) final override;

  f32        _requestTime_s = -1.0f;
private:

  u32        _maxFaceAge_ms = 30000;
  f32        _lastFaceSeenTime_s = -1.0f;
  FaceID_t   _faceID = Vision::UnknownFaceID;
  bool       _hasBlockPose = false;
  Pose3d     _lastBlockPose;
  ObjectID   _robotsBlockID;
  bool       _canRequestGame;

  std::set<ObjectID> _badBlocks;

  std::unique_ptr<BlockWorldFilter>  _blockworldFilter;

  bool FilterBlocks(ProgressionUnlockComponent* unlockComp,
                    AIComponent* aiComp,
                    DockingComponent* dockingComp,
                    const ObservableObject* obj) const;

  const ObservableObject* GetClosestBlock(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  void HandleObservedFace(BehaviorExternalInterface& behaviorExternalInterface,
                          const ExternalInterface::RobotObservedFace& msg);
  void HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg);

};

}
}



#endif
