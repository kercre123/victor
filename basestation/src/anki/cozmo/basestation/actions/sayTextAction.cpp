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
  
  SayTextAction::SayTextAction(Robot& robot, const std::string& text, SayTextStyle style, bool clearOnCompletion)
  : IAction(robot)
  , _text(text)
  , _style(style)
  , _clearOnCompletion(clearOnCompletion)
  , _animation("SayTextAnimation")
  {
    if(ANKI_DEVELOPER_CODE)
    {
      // Only put the text in the action name in dev mode because the text
      // could be a person's name and we don't want that logged for privacy reasons
      _name = "SayText_" + _text + "_Action";
    }
    
    // We don't konw how long the audio after beeing proccessed.
    // If there is an error we will set status state to Failed.
    _timeout_sec = std::numeric_limits<f32>::max();
    
    // Create speech data
    TextToSpeechComponent::PhraseState state = _robot.GetTextToSpeechComponent().CreateSpeech( _text, _style );
    if (state == TextToSpeechComponent::PhraseState::None) {
      PRINT_NAMED_ERROR("SayTextAction.SayTextAction.CreateSpeech", "PhraseState is None");
      _textToSpeechStatus = TextToSpeechStatus::Failed;
    }
  }
  
  SayTextAction::~SayTextAction()
  {
    // Now that we're all done, unload the sounds from memory
    if (_clearOnCompletion) {
      _robot.GetTextToSpeechComponent().ClearLoadedSpeechData(_text, _style);
    }
    
    Util::SafeDelete(_playAnimationAction);
  }
  
  ActionResult SayTextAction::Init()
  {
    if (_textToSpeechStatus == TextToSpeechStatus::Loading) {
      // Update speech state
      TextToSpeechComponent::PhraseState state = _robot.GetTextToSpeechComponent().GetSpeechState( _text, _style );
      switch (state) {
        case TextToSpeechComponent::PhraseState::None:
          _textToSpeechStatus = TextToSpeechStatus::Failed;
          break;
          
        case TextToSpeechComponent::PhraseState::Ready:
          _textToSpeechStatus = TextToSpeechStatus::Ready;
          
        default:
          // Still loading
          break;
      }
    }
    
    // Handle Status state
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
        const bool success = _robot.GetTextToSpeechComponent().PrepareToSay(_text, _style);
        if (!success) {
          PRINT_NAMED_ERROR("SayTextAction.Init.PrepareToSayFailed", "");
          return ActionResult::FAILURE_ABORT;
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
