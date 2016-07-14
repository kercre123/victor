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

#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/audio/audioEventTypes.h"

#define DEBUG_SAYTEXT_ACTION 0


namespace Anki {
namespace Cozmo {
  
  SayTextAction::SayTextAction(Robot& robot, const std::string& text, SayTextStyle style, bool clearOnCompletion)
  : IAction(robot,
            "SayText",
            RobotActionType::SAY_TEXT,
            (u8)AnimTrackFlag::NO_TRACKS)
  , _text(text)
  , _style(style)
  , _clearOnCompletion(clearOnCompletion)
  , _animation("SayTextAnimation")
  {
    if (ANKI_DEVELOPER_CODE) {
      // Only put the text in the action name in dev mode because the text
      // could be a person's name and we don't want that logged for privacy reasons
      SetName("SayText_" + _text);
    }
    
    {
      // TODO: Remove this TEMP FIX
      // Override temporary WWise default which says a random name for say text
      // Once we figure this out, we can change the WWise default ot be "On" and remove this.
      using namespace Audio::GameState;
      _robot.GetRobotAudioClient()->PostGameState(StateGroupType::External_Name, (GenericState)External_Name::External_Name_On);
    }
    
    // Create speech data
    TextToSpeechComponent::SpeechState state = _robot.GetTextToSpeechComponent().CreateSpeech( _text, _style );
    if (state == TextToSpeechComponent::SpeechState::None) {
      PRINT_NAMED_ERROR("SayTextAction.SayTextAction.CreateSpeech", "SpeechState is None");
    }
  }
  
  SayTextAction::~SayTextAction()
  {
    // Now that we're all done, unload the sounds from memory
    if (_clearOnCompletion) {
      _robot.GetTextToSpeechComponent().ClearLoadedSpeechData(_text, _style);
    }
    
    if(_playAnimationAction != nullptr)
    {
      _playAnimationAction->PrepForCompletion();
    }
    Util::SafeDelete(_playAnimationAction);
  }
  
  ActionResult SayTextAction::Init()
  {
    TextToSpeechComponent::SpeechState state = _robot.GetTextToSpeechComponent().GetSpeechState( _text, _style );
    switch (state) {
      case TextToSpeechComponent::SpeechState::Preparing:
      {
        // Can't initialize until text to speech is ready
        if (DEBUG_SAYTEXT_ACTION) {
          PRINT_NAMED_DEBUG("SayTextAction.Init.LoadingTextToSpeech", "");
        }
        return ActionResult::RUNNING;
      }
        break;
        
      case TextToSpeechComponent::SpeechState::Ready:
      {
        // Set Audio data right before action runs
        float duration_ms = 0.0f;
        const bool success = _robot.GetTextToSpeechComponent().PrepareToSay(_text,
                                                                            _style,
                                                                            Audio::GameObjectType::CozmoBus_1,
                                                                            duration_ms);
        if (!success) {
          PRINT_NAMED_ERROR("SayTextAction.Init.PrepareToSayFailed", "");
          return ActionResult::FAILURE_ABORT;
        }
        
        if (duration_ms * 0.001f > _timeout_sec) {
          PRINT_NAMED_DEBUG("SayTextAction.Init.PrepareToSayDurrationTooLong",
                            "Text: %s Style: %hhu", _text.c_str(), _style);
          PRINT_NAMED_ERROR("SayTextAction.Init.PrepareToSayDurrationTooLong", "Durration: %f", duration_ms);
        }
        
        const bool useBuiltInAnim = (AnimationTrigger::Count == _animationTrigger);
        if (useBuiltInAnim) {
          // Make our animation a "live" animation with a single audio keyframe at the beginning
          if (DEBUG_SAYTEXT_ACTION) {
            PRINT_NAMED_DEBUG("SayTextAction.Init.CreatingAnimation", "");
          }
          // Get appropriate audio event for style
          const Audio::GameEvent::GenericEvent audioEvent = _robot.GetTextToSpeechComponent().GetAudioEvent(_style);
          
          _animation.SetIsLive(true);
          _animation.AddKeyFrameToBack(RobotAudioKeyFrame(RobotAudioKeyFrame::AudioRef(audioEvent), 0));
          _playAnimationAction = new PlayAnimationAction(_robot, &_animation);
        } else {
          if (DEBUG_SAYTEXT_ACTION) {
            PRINT_NAMED_DEBUG("SayTextAction.Init.UsingAnimationGroup", "GameEvent=%d (%s)",
                              _animationTrigger, EnumToString(_animationTrigger));
          }
          _playAnimationAction = new TriggerAnimationAction(_robot, _animationTrigger);
        }
        _isAudioReady = true;
        
        return ActionResult::SUCCESS;
      }
        break;
        
      case TextToSpeechComponent::SpeechState::None:
      {
        // Audio load failed
        if (DEBUG_SAYTEXT_ACTION) {
          PRINT_NAMED_DEBUG("SayTextAction.Init.TextToSpeechFailed", "");
        }
        return ActionResult::FAILURE_ABORT;
      }
        break;
    }
  } // Init()
  
  ActionResult SayTextAction::CheckIfDone()
  {
    ASSERT_NAMED(_isAudioReady, "SayTextAction.CheckIfDone.TextToSpeechNotReady");
    
    if (DEBUG_SAYTEXT_ACTION) {
      PRINT_NAMED_DEBUG("SayTextAction.CheckIfDone.UpdatingAnimation", "");
    }
    
    return _playAnimationAction->Update();
  } // CheckIfDone()
  
} // namespace Cozmo
} // namespace Anki
