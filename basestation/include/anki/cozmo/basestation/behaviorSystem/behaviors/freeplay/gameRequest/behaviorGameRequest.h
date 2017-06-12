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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/vision/basestation/trackedFace.h"
#include "clad/types/behaviorTypes.h"
#include <memory>
#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class ObservableObject;

namespace ExternalInterface {
struct RobotDeletedFace;
struct RobotObservedFace;
}

class IBehaviorRequestGame : public IBehavior
{
public:

  IBehaviorRequestGame(Robot& robot, const Json::Value& config);

  // final to ensure subclass does not skip. If you need to override in subclass I suggest another internal one
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const final override;
  
  virtual Result InitInternal(Robot& robot) final override;
  virtual Status UpdateInternal(Robot& robot) final override;

protected:

  using Face = Vision::TrackedFace;
  using FaceID_t = Vision::FaceID_t;
  
  // --------------------------------------------------------------------------------
  // Functions to be overridden by subclasses
  
  virtual Result RequestGame_InitInternal(Robot& robot) = 0;
  virtual Status RequestGame_UpdateInternal(Robot& robot) = 0;
  virtual void HandleGameDeniedRequest(Robot& robot) = 0;
  virtual void RequestGame_StopInternal(Robot& robot) { }

  // --------------------------------------------------------------------------------
  // Utility functions for subclasses
  
  // these send the message (don't do anything with animations)
  void SendRequest(Robot& robot);
  void SendDeny(Robot& robot);

  // the time at which it will be OK to end the behavior (allowing us a delay after the request), or -1
  virtual f32 GetRequestMinDelayComplete_s() const = 0;

  FaceID_t GetFaceID() const { return _faceID; }
  f32 GetLastSeenFaceTime() const {return _lastFaceSeenTime_s;}
    
  virtual u32 GetNumBlocks(const Robot& robot) const;

  // The first call to this will set the block ID, then it will be consistent until the behavior is reset
  ObjectID GetRobotsBlockID(const Robot& robot);

  // If possible, this call will switch the robot to a different block ID. It will add the current ID to a
  // blacklist (which gets reset next time this behavior runs). Returns true if successful
  bool SwitchRobotsBlock(const Robot& robot);

  // Gets the last known pose of the block (useful in case it gets bumped). returns true if it has a block
  // pose, false otherwise
  bool GetLastBlockPose(Pose3d& pose) const;

  bool HasFace(const Robot& robot) const;
  
  // Returns true if HasFace() and returns the pose as argument. Returns false and leaves argument alone otherwise.
  bool GetFacePose(const Robot& robot, Pose3d& facePoseWrtRobotOrigin) const;
  
  virtual void HandleCliffEvent(Robot& robot, const EngineToGameEvent& event) {};

  // --------------------------------------------------------------------------------
  // Functions from IBehavior which aren't exposed to children

  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) final override;
  virtual void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot) final override;
  virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) final override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) final override;
  
  virtual void StopInternal(Robot& robot) final override;

  f32        _requestTime_s = -1.0f;
private:

  u32        _maxFaceAge_ms = 30000;
  f32        _lastFaceSeenTime_s = -1.0f;
  FaceID_t   _faceID = Vision::UnknownFaceID;
  bool       _hasBlockPose = false;
  Pose3d     _lastBlockPose;
  ObjectID   _robotsBlockID;
  UnlockId   _requestID;
  bool       _canRequestGame;

  std::set<ObjectID> _badBlocks;

  std::unique_ptr<BlockWorldFilter>  _blockworldFilter;

  bool FilterBlocks(const Robot* robot, const ObservableObject* obj) const;

  const ObservableObject* GetClosestBlock(const Robot& robot) const;
  
  void HandleObservedFace(const Robot& robot,
                          const ExternalInterface::RobotObservedFace& msg);
  void HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg);

};

}
}



#endif
