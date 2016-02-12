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

  std::string _initialAnimationName;
  std::string _preDriveAnimationName;
  float       _verifyStartTime_s = 0.0f;

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

};

}
}



#endif
