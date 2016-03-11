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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/vision/basestation/trackedFace.h"
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

  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const final override;
  virtual Result InitInternal(Robot& robot, double currentTime_sec) final override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) final override;

protected:

  using Face = Vision::TrackedFace;

  // --------------------------------------------------------------------------------
  // Functions to be overridden by sub classes
  
  virtual Result RequestGame_InitInternal(Robot& robot, double currentTime_sec) = 0;
  virtual Status RequestGame_UpdateInternal(Robot& robot, double currentTime_sec) = 0;
  virtual void HandleGameDeniedRequest(Robot& robot) = 0;

  // --------------------------------------------------------------------------------
  // Utility functions for sub-classes
  
  // these send the message (don't do anything with animations)
  void SendRequest(Robot& robot);
  void SendDeny(Robot& robot);

  // the time at which it will be OK to end the behavior (allowing us a delay after the request), or -1
  virtual f32 GetRequestMinDelayComplete_s() const = 0;

  Face::ID_t GetFaceID() const { return _faceID; }
  f32 GetLastSeenFaceTime() const {return _lastFaceSeenTime_s;}
    
  u32 GetNumBlocks(const Robot& robot) const;
  ObjectID GetRobotsBlockID(const Robot& robot) const;

  // Gets the last known pose of the block (useful in case it gets bumped). returns true if it has a block
  // pose, false otherwise
  bool GetLastBlockPose(Pose3d& pose) const;

  bool HasFace(const Robot& robot) const;
  bool GetFacePose(const Robot& robot, Pose3d& facePose) const;

  // --------------------------------------------------------------------------------
  // Functions from IBehavior which aren't exposed to children

  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) final override;
  virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) final override;

  f32        _requestTime_s = -1.0f;
private:

  u32        _maxFaceAge_ms = 30000;
  f32        _lastFaceSeenTime_s = -1.0f;
  Face::ID_t _faceID = Face::UnknownFace;
  bool       _hasBlockPose = false;
  Pose3d     _lastBlockPose;

  ObservableObject* GetClosestBlock(const Robot& robot) const;
  
  void HandleObservedFace(const Robot& robot,
                          const ExternalInterface::RobotObservedFace& msg);
  void HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg);

};

}
}



#endif
