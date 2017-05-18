/**
 * File: behaviorBouncer.h
 *
 * Description: Cozmo plays a classic retro arcade game
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__
#define __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/smartFaceId.h"

// Forward declarations
namespace Anki {
  namespace Vision {
    class Image;
    class TrackedFace;
  }
}

namespace Anki {
namespace Cozmo {
  
class BehaviorBouncer : public IBehavior
{
public:
  
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorBouncer(Robot& robot, const Json::Value& config);
  
  // IBehavior interface
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReq) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
  virtual Result InitInternal(Robot& robot) override;
  virtual BehaviorStatus UpdateInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;
  
private:

  // Helper types
  enum class State {
    Init,                   // Initialize everything and play starting animations
    Timeout,                // Timeout expired
    Complete
  };
  
  // Member variables
  State _state = State::Init;
  
  // This must be mutable to retain state from trigger prerequisites
  mutable SmartFaceID _target;
  
  // Width and height of display, in pixels
  float _displayWidth_px = 0;
  float _displayHeight_px = 0;
  
  // Player stats
  u32 _playerHits = 0;
  u32 _playerMisses = 0;
  
  // Paddle position, in screen coordinates
  float _paddlePosX = 0.f;
  float _paddlePosY = 0.f;
  float _paddleSpeedX = 0.f;
  
  // Ball position, in screen coordinates
  float _ballPosX = 0.f;
  float _ballPosY = 0.f;
  
  // Ball speed, in screen coordinates per tick
  float _ballSpeedX = 0.f;
  float _ballSpeedY = 0.f;
  
  // Did ball hit paddle or floor?
  bool _ballHitPaddle = false;
  bool _ballHitFloor = false;
  
  bool _isSoundActionInProgress = false;
  bool _isFaceActionInProgress = false;
  
  // Update helpers
  void UpdatePaddle(const Vision::TrackedFace * face);
  void UpdateBall();
  void UpdateSound(Robot& robot);
  void UpdateDisplay(Robot& robot);
  
  // Draw helpers
  void DrawPaddle(Vision::Image& image);
  void DrawBall(Vision::Image& image);
  void DrawScore(Vision::Image& image);
  
  // Action callbacks
  void SoundActionComplete();
  void FaceActionComplete();
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__
