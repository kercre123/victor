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

#define DEBUG_SAYTEXT_ACTION 0

namespace Anki {
namespace Cozmo {
  
  SayTextAction::SayTextAction(Robot& robot, const std::string& text, SayTextStyle style)
  : IAction(robot)
  , _text(text)
  , _style(style)
  , _animation("SayTextAnimation")
  {
    if(ANKI_DEVELOPER_CODE)
    {
      // Only put the text in the action name in dev mode because the text
      // could be a person's name and we don't want that logged for privacy reasons
      _name = "SayText_" + _text + "_Action";
    }
    
    // Create and/or load speech data
    TextToSpeechComponent::CompletionFunc callback = [this] (bool success,
                                                              const std::string& text,
                                                              const std::string& fileName)
    {
      if(DEBUG_SAYTEXT_ACTION){
        PRINT_NAMED_DEBUG("SayTextAction.CompletionCallback", "success=%d, text=%s, filename=%s",
                          success, text.c_str(), fileName.c_str());
      }
      if (success) {
        _textToSpeechStatus = TextToSpeechStatus::Ready;
      }
      else {
        PRINT_NAMED_ERROR("SayTextAction.SayTextAction.LoadSpeechDataFailed", "");
        _textToSpeechStatus = TextToSpeechStatus::Failed;
      }
    };
    
    if(DEBUG_SAYTEXT_ACTION){
      PRINT_NAMED_DEBUG("SayTextAction.Constructor.LoadingSpeechData", "");
    }
    _robot.GetTextToSpeechComponent().LoadSpeechData(_text, _style, callback);
  }
  
  SayTextAction::~SayTextAction()
  {
    // Now that we're all done, unload the sounds from memory
    _robot.GetTextToSpeechComponent().UnloadSpeechData(_text, _style);
    
    Util::SafeDelete(_playAnimationAction);
  }
  
  ActionResult SayTextAction::Init()
  {
    switch(_textToSpeechStatus)
    {
      case TextToSpeechStatus::Loading:
        // Can't initialize until text to speech is ready
        if(DEBUG_SAYTEXT_ACTION){
          PRINT_NAMED_DEBUG("SayTextAction.Init.LoadingTextToSpeech", "");
        }
        return ActionResult::RUNNING;
    
      case TextToSpeechStatus::Failed:
        // Audio load failed
        if(DEBUG_SAYTEXT_ACTION){
          PRINT_NAMED_DEBUG("SayTextAction.Init.TextToSpeechFailed", "");
        }
        return ActionResult::FAILURE_ABORT;
        
      case TextToSpeechStatus::Ready:
      {
        // Set Audio data right before action runs
        float duration_ms = 0.0;
        const bool success = _robot.GetTextToSpeechComponent().PrepareToSay(_text, _style, duration_ms);
        if (!success) {
          PRINT_NAMED_ERROR("SayTextAction.Init.PrepareToSayFailed", "");
          return ActionResult::FAILURE_ABORT;
        }
        
        // Make timeout relative to the length of the sound, in seconds, now that we know its duration
        _timeout_sec = 3.f * .001f * duration_ms; // "3" is a fudge factor, .001 to convert from ms to sec
        
        if(DEBUG_SAYTEXT_ACTION){
          PRINT_NAMED_DEBUG("SayTextAction.Init.TextToSpeechReady", "Got duration=%.2fms, timeout=%.1fsec",
                            duration_ms, _timeout_sec);
        }

        const bool useBuiltInAnim = (GameEvent::Count == _gameEvent);
        if(useBuiltInAnim) {
          // Make our animation a "live" animation with a single audio keyframe at the beginning
          if(DEBUG_SAYTEXT_ACTION){
            PRINT_NAMED_DEBUG("SayTextAction.Init.CreatingAnimation", "");
          }
          _animation.SetIsLive(true);
          _animation.AddKeyFrameToBack(RobotAudioKeyFrame(Audio::GameEvent::GenericEvent::Vo_Coz_External_Play, 0));
          _playAnimationAction = new PlayAnimationAction(_robot, &_animation);
        } else {
          if(DEBUG_SAYTEXT_ACTION){
            PRINT_NAMED_DEBUG("SayTextAction.Init.UsingAnimationGroup", "GameEvent=%hhu (%s)",
                              _gameEvent, EnumToString(_gameEvent));
          }
          _playAnimationAction = new PlayAnimationGroupAction(_robot, _gameEvent);
        }
        return ActionResult::SUCCESS;
      }
    }
    
  } // Init()
  
  ActionResult SayTextAction::CheckIfDone()
  {
    ASSERT_NAMED(_textToSpeechStatus == TextToSpeechStatus::Ready,
                 "SayTextAction.CheckIfDone.TextToSpeechNotReady");
    
    if(DEBUG_SAYTEXT_ACTION){
      PRINT_NAMED_DEBUG("SayTextAction.CheckIfDone.UpdatingAnimation", "");
    }
    
    return _playAnimationAction->Update();
    
  } // CheckIfDone()
  
} // namespace Cozmo
} // namespace Anki
