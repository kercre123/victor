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
, _ttsOperationId(TextToSpeechComponent::kInvalidOperationId)
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
, _ttsOperationId(TextToSpeechComponent::kInvalidOperationId)
, _animation("SayTextAnimation")
{
  if (DEBUG_SAYTEXT_ACTION) {
    PRINT_CH_INFO(kLocalLogChannel,
                  "SayTextAction.Init",
                  "Text '%s' SayTextIntent '%s'",
                  Util::HidePersonallyIdentifiableInfo(_text.c_str()), EnumToString(intent));
  }
  
  // Create text for 'Intent'
  // Set voice style & length scalar to generate TtS
  bool isRandom   = false;
  float minScalar = 0.f;
  float maxScalar = 0.f;
  float stepSize  = 0.f;
  const uint8_t charLengthThreshold = 7;
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
      stepSize = 0.05f;
      isRandom = true;
      if (text.length() < charLengthThreshold) {
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
      
    case SayTextIntent::Name_FirstIntroduction_1:
    {
      // Say name very slow the first time
      _style = SayTextVoiceStyle::CozmoProcessing;
      stepSize = 0.1f;
      isRandom = true;
      if (text.length() < charLengthThreshold) {
        // Short names
        minScalar = 2.35f;
        maxScalar = 2.65f;
      }
      else {
        // Long names
        minScalar = 2.25f;
        maxScalar = 2.55f;
      }
      break;
    }
      
    case SayTextIntent::Name_FirstIntroduction_2:
    {
      // Say name slightly faster then the first time
      _style = SayTextVoiceStyle::CozmoProcessing;
      stepSize = 0.1f;
      isRandom = true;
      if (text.length() < charLengthThreshold) {
        // Short names
        minScalar = 2.0f;
        maxScalar = 2.2f;
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
    // Break scalar range into step sizes
    ASSERT_NAMED(stepSize > 0.f, "SayTextAction.SayTextAction.stepSize.IsZero");
    // (Scalar Range / stepSize) + 1 = number of total possible steps
    const uint8_t stepCount = ((maxScalar - minScalar) / stepSize) + 1;
    const auto randStep = robot.GetRNG().RandInt(stepCount);
    _durationScalar = minScalar + (stepSize * randStep);
  }
  GenerateTtsAudio();
} // SayTextAction()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::~SayTextAction()
{
  // Now that we're all done, cleanup possible audio data leaks caused by action or animations being aborted. This is
  // safe to call for success as well.
  _robot.GetTextToSpeechComponent().CleanupAudioEngine(_ttsOperationId);
  
  if(_playAnimationAction != nullptr) {
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
      const bool success = _robot.GetTextToSpeechComponent().PrepareAudioEngine(_ttsOperationId,
                                                                                duration_ms);
      if (success) {
        // Don't need to be responsible for audio data after successfully TextToSpeechComponent().PrepareAudioEngine()
        _ttsOperationId = TextToSpeechComponent::kInvalidOperationId;
      }
      else {
        PRINT_NAMED_ERROR("SayTextAction.Init.PrepareAudioEngine.Failed", "");
        return ActionResult::FAILURE_ABORT;
      }
      
      if (duration_ms * 0.001f > _timeout_sec) {
        PRINT_NAMED_ERROR("SayTextAction.Init.PrepareAudioEngine.DurrationTooLong", "Duration: %f", duration_ms);
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
        _playAnimationAction = new TriggerLiftSafeAnimationAction(_robot, _animationTrigger);
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
  // Be careful with putting text in the action name because it could be a player name, which is PII
  SetName(std::string("SayText_") + Util::HidePersonallyIdentifiableInfo(_text.c_str()));
  
  // Create speech data
  _ttsOperationId = _robot.GetTextToSpeechComponent().CreateSpeech(_text, _style, _durationScalar);
  if (TextToSpeechComponent::kInvalidOperationId == _ttsOperationId) {
    PRINT_NAMED_ERROR("SayTextAction.SayTextAction.CreateSpeech", "SpeechState is None");
  }
} // GenerateTtsAudio()

} // namespace Cozmo
} // namespace Anki
