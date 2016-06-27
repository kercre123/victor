/**
 * File: sayTextAction.h
 *
 * Author: Andrew Stein
 * Date:   4/23/2016
 *
 * Description: Defines an action for having Cozmo "say" a text string.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Cozmo_Actions_SayTextAction_H__
#define __Anki_Cozmo_Actions_SayTextAction_H__

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "clad/types/sayTextStyles.h"
#include "clad/types/animationTrigger.h"


namespace Anki {
namespace Cozmo {

  class SayTextAction : public IAction
  {
  public:
    
    SayTextAction(Robot& robot, const std::string& text, SayTextStyle style, bool clearOnCompletion);
    
    virtual ~SayTextAction();
    
    virtual const std::string& GetName() const override { return _name; }
    virtual RobotActionType GetType() const override { return RobotActionType::SAY_TEXT; }
    
    virtual f32 GetTimeoutInSeconds() const override { return _timeout_sec; }
    
    virtual u8 GetTracksToLock() const override { 
      return (u8)AnimTrackFlag::NO_TRACKS;
    }
    
    // Use a the animation group tied to a specific GameEvent.
    // Use GameEvent::Count to use built-in animation (default).
    // The animation group should contain animations that have the special audio
    // keyframe for Audio::GameEvent::GenericEvent::Vo_Coz_External_Play.
    void SetAnimationTrigger(AnimationTrigger trigger) { _animationTrigger = trigger; }
    
  protected:
    
    virtual ActionResult Init() override;
    virtual ActionResult CheckIfDone() override;

  private:
    
    std::string               _text;
    std::string               _name                 = "SayTextAction";
    SayTextStyle              _style;
    bool                      _clearOnCompletion    = true;
    bool                      _isAudioReady         = false;
    Animation                 _animation;
    AnimationTrigger          _animationTrigger     = AnimationTrigger::Count; // Count == use built-in animation
    IActionRunner*            _playAnimationAction  = nullptr;
    f32                       _timeout_sec          = 30.f;
    
  }; // class SayTextAction


} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Actions_SayTextAction_H__ */
