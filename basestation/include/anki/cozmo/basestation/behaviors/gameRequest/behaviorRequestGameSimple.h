/**
 * File: behaviorRequestGameSimple.h
 *
 * Author: Brad Neuman
 * Created: 2016-02-03
 *
 * Description: re-usable game request behavior which works with or without blocks
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_GameRequest_BehaviorRequestGameSimple_H__
#define __Cozmo_Basestation_Behaviors_GameRequest_BehaviorRequestGameSimple_H__

#include "anki/cozmo/basestation/behaviors/gameRequest/behaviorGameRequest.h"
#include "clad/types/pathMotionProfile.h"
#include <string>

namespace Anki {
namespace Cozmo {

class BehaviorRequestGameSimple : public IBehaviorRequestGame
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRequestGameSimple(Robot& robot, const Json::Value& config);

public:

  virtual ~BehaviorRequestGameSimple() {}

protected:
  virtual Result RequestGame_InitInternal(Robot& robot, double currentTime_sec) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
  virtual void   StopInternal(Robot& robot, double currentTime_sec) override;
  virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override;

  virtual void HandleGameDeniedRequest(Robot& robot) override;
  
private:

  // ========== Members ==========

  enum class State {
    PlayingInitialAnimation,
    FacingBlock,
    PlayingPreDriveAnimation,
    PickingUpBlock,
    DrivingToFace,
    PlacingBlock,
    LookingAtFace,
    VerifyingFace,
    PlayingRequstAnim,
    TrackingFace,
    PlayingDenyAnim,
  };

  State _state = State::PlayingInitialAnimation;

  // there are two sets of values, based on whether there are 0 or more blocks available (at the time of Init)
  struct ConfigPerNumBlocks {
    void LoadFromJson(const Json::Value& config);
    std::string initialAnimationName;
    std::string preDriveAnimationName;
    std::string requestAnimationName;
    std::string denyAnimationName;
    float       minRequestDelay;
    float       scoreFactor;
  };

  ConfigPerNumBlocks _zeroBlockConfig;
  ConfigPerNumBlocks _oneBlockConfig;

  PathMotionProfile _driveToPickupProfile;
  PathMotionProfile _driveToPlaceProfile;

  float _driveToPlacePoseThreshold_mm;
  float _driveToPlacePoseThreshold_rads;

  float _afterPlaceBackupDist_mm;
  float _afterPlaceBackupSpeed_mmps;

  ConfigPerNumBlocks* _activeConfig = nullptr;
  
  float  _verifyStartTime_s = 0.0f;

  Pose3d _faceInteractionPose;
  bool   _hasFaceInteractionPose = false;

  void SetState_internal(State state, const std::string& stateName);

  void TransitionToPlayingInitialAnimation(Robot& robot);
  void TransitionToFacingBlock(Robot& robot);
  void TransitionToPlayingPreDriveAnimation(Robot& robot);
  void TransitionToPickingUpBlock(Robot& robot);
  void TransitionToDrivingToFace(Robot& robot);
  void TransitionToPlacingBlock(Robot& robot);
  void TransitionToLookingAtFace(Robot& robot);
  void TransitionToVerifyingFace(Robot& robot);
  void TransitionToPlayingRequstAnim(Robot& robot);
  void TransitionToTrackingFace(Robot& robot);
  void TransitionToPlayingDenyAnim(Robot& robot);

  bool GetFaceInteractionPose(Robot& robot, Pose3d& pose);
  void ComputeFaceInteractionPose(Robot& robot);

};

}
}



#endif
