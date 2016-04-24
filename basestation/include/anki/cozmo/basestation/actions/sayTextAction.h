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
#include "clad/types/actionTypes.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"


namespace Anki {
namespace Cozmo {

  class SayTextAction : public IAction
  {
  public:
    SayTextAction(Robot& robot, const std::string& text);
    
    virtual ~SayTextAction();
    
    virtual const std::string& GetName() const override { return _name; }
    virtual RobotActionType GetType() const override { return RobotActionType::SAY_TEXT; }
    
    // TODO: Use duration of the sound?
    virtual f32 GetTimeoutInSeconds() const override { return 5.f; }
    
    virtual u8 GetTracksToLock() const override { return (u8)AnimTrackFlag::NO_TRACKS; }
    
  protected:
    
    virtual ActionResult Init() override;
    virtual ActionResult CheckIfDone() override;

  private:
    std::string               _text = "SayTextAction";
    std::string               _name;
    PlayAnimationAction       _playAnimationAction;
    
  }; // class SayTextAction


} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Actions_SayTextAction_H__ */
