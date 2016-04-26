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
  , _animation("SayTextAnimation")
  , _playAnimationAction(robot, &_animation)
  {
    if(ANKI_DEVELOPER_CODE)
    {
      // Only put the text in the action name in dev mode because the text
      // could be a person's name and we don't want that logged for privacy reasons
      _name = "SayText_" + _text + "_Action";
    }
    
    // Make our animation a "live" animation with a single audio keyframe at the beginning
    _animation.SetIsLive(true);
    _animation.AddKeyFrameToBack(RobotAudioKeyFrame(Audio::GameEvent::GenericEvent::Vo_Coz_External_Play, 0));
    
    // Create and/or load speech data
    TextToSpeechController::CompletionFunc callback = [this] (bool success,
                                                              const std::string& text,
                                                              const std::string& fileName)
    {
      if (success) {
        _textToSpeechStatus = TextToSpeechStatus::Ready;
      }
      else {
        PRINT_NAMED_ERROR("SayTextAction.SayTextAction.LoadSpeechDataFailed", "");
        _textToSpeechStatus = TextToSpeechStatus::Failed;
      }
    };
    
    _robot.GetTextToSpeechController().LoadSpeechData(_text, _style, callback);
  }
  
  SayTextAction::~SayTextAction()
  {

  }
  
  ActionResult SayTextAction::Init()
  {
    // Set Audio data right before action runs
    float duration_ms = 0.0;  // FIXME: hook up to action time out
    const bool success = _robot.GetTextToSpeechController().PrepareToSay(_text, _style, duration_ms);
    if (!success) {
      PRINT_NAMED_ERROR("SayTextAction.Init.PrepareToSayFailed", "");
      return ActionResult::FAILURE_ABORT;
    }
    
    return ActionResult::SUCCESS;
  } // Init()
  
  ActionResult SayTextAction::CheckIfDone()
  {
    switch(_textToSpeechStatus)
    {
      case TextToSpeechStatus::Ready:
        // Play the animation action once the audio is ready
        return _playAnimationAction.Update();
        
      case TextToSpeechStatus::Loading:
        // Wait for audio to load
        return ActionResult::RUNNING;
        
      case TextToSpeechStatus::Failed:
        // Audio load failed
        return ActionResult::FAILURE_ABORT;
        
    } // switch(_textToSpeechStatus)
    
  } // CheckIfDone()
  
  
} // namespace Cozmo
} // namespace Anki
