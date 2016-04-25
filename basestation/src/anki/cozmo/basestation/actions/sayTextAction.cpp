/**
 * File: animActions.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements animation and audio cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "sayTextAction.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/audio/audioEventTypes.h"

namespace Anki {
namespace Cozmo {

  SayTextAction::SayTextAction(Robot& robot, const std::string& text, SayTextStyle style)
  : IAction(robot)
  , _text(text)
  , _style(style)
  , _playAnimationGroupAction(robot, "SayText") // TODO: need to define the group
  {
    if(ANKI_DEVELOPER_CODE)
    {
      // Only put the text in the action name in dev mode because the text
      // could be a person's name and we don't want that logged for privacy reasons
      _name = "SayText_" + _text + "_Action";
    }
    
    // TODO: Create speech here
  }
  
  SayTextAction::~SayTextAction()
  {

  }
  
  ActionResult SayTextAction::Init()
  {
    _isTextToSpeechReady = false;
    TextToSpeech::ReadyCallback callback = [this]()
    {
      _isTextToSpeechReady = true;
    };
    
    Result result = _robot.GetTextToSpeech().PrepareToSay(_text, _style, callback);
    if(RESULT_OK != result) {
      PRINT_NAMED_ERROR("SayTextAction.Init.PrepareToSayFailed", "");
      return ActionResult::FAILURE_ABORT;
    }
    
    return ActionResult::SUCCESS;
  }
  
  ActionResult SayTextAction::CheckIfDone()
  {
    if(_isTextToSpeechReady) {
      return _playAnimationGroupAction.Update();
    } else {
      return ActionResult::RUNNING;
    }
  }
  
  
} // namespace Cozmo
} // namespace Anki
