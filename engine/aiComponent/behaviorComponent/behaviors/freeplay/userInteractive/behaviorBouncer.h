/**
 * File: behaviorBouncer.h
 *
 * Description: Cozmo plays a classic retro arcade game
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__
#define __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

// Forward declarations
namespace Anki {
  namespace Vision {
    class Image;
    class TrackedFace;
  }
}

namespace Anki {
namespace Cozmo {
  
class BehaviorBouncer : public ICozmoBehavior
{
public:
  
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorBouncer(const Json::Value& config);
  
  // ICozmoBehavior interface
  virtual bool WantsToBeActivatedBehavior() const override;
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
private:

  // Helper types
  enum class BouncerState {
    Init,                   // Initialize everything
    GetIn,
    IdeaToPlay,
    RequestToPlay,
    WaitToPlay,
    Play,
    Timeout,                // Timeout expired
    ShowScore,
    GetOut,
    Complete
  };
  
  // Member variables
  BouncerState _state = BouncerState::Init;
  
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
  
  // Did ball hit the floor?
  bool _ballHitFloor = false;
  
  // State management
  const char * EnumToString(const BouncerState& state);
  void TransitionToState(const BouncerState& state);
  
  // Update helpers
  void UpdatePaddle(const Vision::TrackedFace * face);
  void UpdateBall();
  void UpdateSound();
  void UpdateDisplay();
  
  // Draw helpers
  void DrawPaddle(Vision::Image& image);
  void DrawBall(Vision::Image& image);
  void DrawScore(Vision::Image& image);
  
  // Action helpers
  void StartAnimation(const AnimationTrigger& animationTrigger);
  void StartAnimation(const AnimationTrigger& animationTrigger, const BouncerState& nextState);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__
