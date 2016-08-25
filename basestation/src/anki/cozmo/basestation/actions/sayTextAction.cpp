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
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"


#define DEBUG_SAYTEXT_ACTION 0


namespace Anki {
namespace Cozmo {

const char* kLocalLogChannel = "Actions";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::SayTextAction(Robot& robot, const std::string& text, const SayTextVoiceStyle style, const float durationScalar)
: IAction(robot,
          "SayText",
          RobotActionType::SAY_TEXT,
          (u8)AnimTrackFlag::NO_TRACKS)
, _text(text)
, _style(style)
, _durationScalar(durationScalar)
, _animation("SayTextAnimation")

{
  GenerateTtsAudio();
} // SayTextAction()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::SayTextAction(Robot& robot, const std::string& text, const SayTextIntent intent)
: IAction(robot,
          "SayText",
          RobotActionType::SAY_TEXT,
          (u8)AnimTrackFlag::NO_TRACKS)
, _text(text)
, _animation("SayTextAnimation")
{
  // Create text for 'Intent'
  // Set voice style & length scalar to generate TtS
  bool isRandom = false;
  float minScalar = 0.f;
  float maxScalar = 0.f;
  switch (intent) {
    case SayTextIntent::UnProcessed:
    {
      // No processing play as is
      _style = SayTextVoiceStyle::UnProcessed;
      _durationScalar = 1.2f;
      break;
    }
      
    case SayTextIntent::Text:
    {
      // Use Cozmo Processing with default speed
      _style = SayTextVoiceStyle::CozmoProcessing;
      _durationScalar = 1.8f;
      break;
    }
      
    case SayTextIntent::Name_Normal:
    {
      // Normal name
      _style = SayTextVoiceStyle::CozmoProcessing;
      isRandom = true;
      if (text.length() < 7) {
        // Short names
        minScalar = 1.8f;
        maxScalar = 2.1f;
      }
      else {
        // Long names
        minScalar = 1.7f;
        maxScalar = 2.f;
      }
      break;
    }
      
    case SayTextIntent::Name_FirstIntroduction:
    {
      // Say name slower
      _style = SayTextVoiceStyle::CozmoProcessing;
      isRandom = true;
      if (text.length() < 7) {
        // Short names
        minScalar = 2.0f;
        maxScalar = 2.3f;
      }
      else {
        // Long names
        minScalar = 1.9f;
        maxScalar = 2.1f;
      }
      break;
    }
    
    case SayTextIntent::Count:
      ASSERT_NAMED(SayTextIntent::Count == intent, "SayTextAction.ActionStyle.SayTextActionStyle::Count.IsInvalid");
      break;
  }
  
  // If random create
  if (isRandom) {
    _durationScalar = Util::numeric_cast<float>(robot.GetRNG().RandDblInRange(minScalar, maxScalar));
  }
  GenerateTtsAudio();
} // SayTextAction()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::~SayTextAction()
{
  // Now that we're all done, remove form the audio engine and unload the sounds from memory
  _robot.GetTextToSpeechComponent().CompletedSpeech();
  _robot.GetTextToSpeechComponent().ClearOperationData(_ttsOperationId);
  
  if(_playAnimationAction != nullptr)
  {
    _playAnimationAction->PrepForCompletion();
  }
  Util::SafeDelete(_playAnimationAction);
} // ~SayTextAction()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult SayTextAction::Init()
{
  TextToSpeechComponent::AudioCreationState state = _robot.GetTextToSpeechComponent().GetOperationState(_ttsOperationId);
  switch (state) {
    case TextToSpeechComponent::AudioCreationState::Preparing:
    {
      // Can't initialize until text to speech is ready
      if (DEBUG_SAYTEXT_ACTION) {
        PRINT_CH_INFO(kLocalLogChannel, "SayTextAction.Init.LoadingTextToSpeech", "");
      }
      return ActionResult::RUNNING;
    }
      break;
      
    case TextToSpeechComponent::AudioCreationState::Ready:
    {
      // Set Audio data right before action runs
      float duration_ms = 0.0f;
      // FIXME: Need to way to get other Audio GameObjs
      const bool success = _robot.GetTextToSpeechComponent().PrepareSpeech(_ttsOperationId,
                                                                           Audio::GameObjectType::CozmoBus_1,
                                                                           duration_ms);

      if (!success) {
        PRINT_NAMED_ERROR("SayTextAction.Init.PrepareSpeech.Failed", "");
        return ActionResult::FAILURE_ABORT;
      }
      
      if (duration_ms * 0.001f > _timeout_sec) {
        PRINT_NAMED_ERROR("SayTextAction.Init.PrepareSpeech.DurrationTooLong", "Durration: %f", duration_ms);
      }
      
      const bool useBuiltInAnim = (AnimationTrigger::Count == _animationTrigger);
      if (useBuiltInAnim) {
        // Make our animation a "live" animation with a single audio keyframe at the beginning
        if (DEBUG_SAYTEXT_ACTION) {
          PRINT_CH_INFO(kLocalLogChannel, "SayTextAction.Init.CreatingAnimation", "");
        }
        // Get appropriate audio event for style
        const Audio::GameEvent::GenericEvent audioEvent = _robot.GetTextToSpeechComponent().GetAudioEvent(_style);
        
        _animation.SetIsLive(true);
        _animation.AddKeyFrameToBack(RobotAudioKeyFrame(RobotAudioKeyFrame::AudioRef(audioEvent), 0));
        _playAnimationAction = new PlayAnimationAction(_robot, &_animation);
      } else {
        if (DEBUG_SAYTEXT_ACTION) {
          PRINT_CH_INFO(kLocalLogChannel,
                        "SayTextAction.Init.UsingAnimationGroup", "GameEvent=%d (%s)",
                        _animationTrigger, EnumToString(_animationTrigger));
        }
        _playAnimationAction = new TriggerAnimationAction(_robot, _animationTrigger);
      }
      _isAudioReady = true;
      
      return ActionResult::SUCCESS;
    }
      break;
      
    case TextToSpeechComponent::AudioCreationState::None:
    {
      // Audio load failed
      if (DEBUG_SAYTEXT_ACTION) {
        PRINT_CH_INFO(kLocalLogChannel, "SayTextAction.Init.TextToSpeechFailed", "");
      }
      return ActionResult::FAILURE_ABORT;
    }
      break;
  }
} // Init()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult SayTextAction::CheckIfDone()
{
  ASSERT_NAMED(_isAudioReady, "SayTextAction.CheckIfDone.TextToSpeechNotReady");
  
  if (DEBUG_SAYTEXT_ACTION) {
    PRINT_CH_INFO(kLocalLogChannel, "SayTextAction.CheckIfDone.UpdatingAnimation", "");
  }
  
  return _playAnimationAction->Update();
} // CheckIfDone()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SayTextAction::GenerateTtsAudio()
{
  if (ANKI_DEVELOPER_CODE) {
    // Only put the text in the action name in dev mode because the text
    // could be a person's name and we don't want that logged for privacy reasons
    SetName("SayText_" + _text);
  }
  
  // Create speech data
  _ttsOperationId = _robot.GetTextToSpeechComponent().CreateSpeech(_text, _style, _durationScalar);
  if (TextToSpeechComponent::kInvalidOperationId == _ttsOperationId) {
    PRINT_NAMED_ERROR("SayTextAction.SayTextAction.CreateSpeech", "SpeechState is None");
  }
} // GenerateTtsAudio()

} // namespace Cozmo
} // namespace Anki
