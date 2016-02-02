/**
 * File: behaviorRequestGameZeroBlocks.h
 *
 * Author: Brad Neuman
 * Created: 2016-01-27
 *
 * Description: re-usable behavior to request to play a game by turning to a face and playing an
 *              animation. This handles the case where there are no known blocks
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __COZMO_BASESTATION_BEHAVIORS_GAMEREQUEST_BEHAVIORREQUESTGAMEZEROBLOCKS_H__
#define __COZMO_BASESTATION_BEHAVIORS_GAMEREQUEST_BEHAVIORREQUESTGAMEZEROBLOCKS_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/vision/basestation/trackedFace.h"
#include <string>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
struct RobotCompletedAction;
struct RobotDeletedFace;
struct RobotObservedFace;
}

class BehaviorRequestGameZeroBlocks : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRequestGameZeroBlocks(Robot& robot, const Json::Value& config);
  
public:
  
  virtual ~BehaviorRequestGameZeroBlocks();
    
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;
  virtual void   StopInternal(Robot& robot, double currentTime_sec) override;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) override;

private:

  using Face = Vision::TrackedFace;
  
  void StartActing(Robot& robot, IActionRunner* action);

  void HandleObservedFace(const Robot& robot,
                          const ExternalInterface::RobotObservedFace& msg);
  void HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg);
  void HandleActionCompleted(Robot& robot,
                             const ExternalInterface::RobotCompletedAction& msg);

  // ========== Members ==========

  enum class State {
    LookingAtFace,
    PlayingRequstAnim,
    TrackingFace,
    PlayingDenyAnim,
  };

  State         _state = State::LookingAtFace;

  // params

  std::string   _requestAnimationName;
  std::string   _denyAnimationName;

  u32           _maxFaceAge_ms = 30000;
  f32           _minRequestDelay_s = 3.0f;

  // internal variables
  
  Face::ID_t    _faceID = Face::UnknownFace;
  
  f32           _requestTime_s = -1.0f;
  
  u32           _lastActionTag = 0;
  bool          _isActing = false;
};
  

} // namespace Cozmo
} // namespace Anki

#endif
