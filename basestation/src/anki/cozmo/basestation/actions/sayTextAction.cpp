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

# define PLACEHOLDER_CODE 1
  
# if PLACEHOLDER_CODE
  static Animation tempAnim;
  static bool animSent = false;
# endif
  
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
    
    // Create and/or load speech data
    TextToSpeechController::CompletionFunc callback = [this] (bool success,
                                                              const std::string& text,
                                                              const std::string& fileName)
    {
      if (success) {
        _isTextToSpeechReady = true;
      }
      else {
        PRINT_NAMED_ERROR("SayTextAction.SayTextAction.LoadSpeechData", "");
        // Abort Action
      }
    };
    
    _robot.GetTextToSpeechController().LoadSpeechData(_text, _style, callback);
  }
  
  SayTextAction::~SayTextAction()
  {

  }
  
  ActionResult SayTextAction::Init()
  {
    // Run Action
    // Set Audio data right before action runs
    float duration_ms = 0.0;  // FIXME: hook up to action time out
    const bool success = _robot.GetTextToSpeechController().PrepareToSay(_text, _style, duration_ms);
    if (!success) {
      return ActionResult::FAILURE_ABORT;
    }
    
    
#   if PLACEHOLDER_CODE
    tempAnim.SetIsLive(true);
    tempAnim.AddKeyFrameToBack(RobotAudioKeyFrame(Audio::GameEvent::GenericEvent::Vo_Coz_External_Play, 0));
    animSent = false;
#   endif
    
    return ActionResult::SUCCESS;
  }
  
  ActionResult SayTextAction::CheckIfDone()
  {
    if(_isTextToSpeechReady) {
#     if PLACEHOLDER_CODE
      if(!animSent) {
        _robot.GetAnimationStreamer().SetStreamingAnimation(_robot, &tempAnim);
        animSent = true;
        return ActionResult::RUNNING;
      } else if(_robot.IsAnimating()) {
        return ActionResult::RUNNING;
      } else {
        return ActionResult::SUCCESS;
      }
#     else
      return _playAnimationGroupAction.Update();
#     endif
    } else {
      return ActionResult::RUNNING;
    }
  }
  
  
} // namespace Cozmo
} // namespace Anki
