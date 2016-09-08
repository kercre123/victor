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
#include "util/math/math.h"
#include "util/random/randomGenerator.h"


#define DEBUG_SAYTEXT_ACTION 0


namespace Anki {
namespace Cozmo {

const char* kLocalLogChannel = "Actions";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::SayTextAction(Robot& robot,
                             const std::string& text,
                             const SayTextVoiceStyle style,
                             const float durationScalar,
                             const float voicePitch)
: IAction(robot,
          "SayText",
          RobotActionType::SAY_TEXT,
          (u8)AnimTrackFlag::NO_TRACKS)
, _text(text)
, _style(style)
, _durationScalar(durationScalar)
, _voicePitch(voicePitch)
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
  
  // Helper Struct
  struct RandomRange {
    float min = 0.f;
    float max = 0.f;
    RandomRange(){}
    RandomRange(float min, float max)
    : min(min)
    , max(max)
    { ASSERT_NAMED(min <= max, "SayTextAction.RandomRange.SetRange.Max.IsLessThan.Min"); }
    void SetRange(float minVal, float maxVal)
    {
      ASSERT_NAMED(minVal <= maxVal, "SayTextAction.RandomRange.SetRange.MaxVal.IsLessThan.MinVal");
      min = minVal;
      max = maxVal;
    }
  };
  
  // Create text for 'Intent'
  // Set voice style & length scalar to generate TtS
  bool isRandom   = false;
  RandomRange durationRange;
  RandomRange pitchRange;
  float stepSize  = 0.f;
  const uint8_t kCharLengthThreshold = 7;
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
      pitchRange.SetRange(0.f, 0.3f);
      stepSize = 0.05f;
      isRandom = true;
      if (text.length() < kCharLengthThreshold) {
        // Short names
        durationRange.SetRange(1.8f, 2.1f);
      }
      else {
        // Long names
        durationRange.SetRange(1.7f, 2.f);
      }
      break;
    }
      
    case SayTextIntent::Name_FirstIntroduction_1:
    {
      // Say name very slow the first time
      _style = SayTextVoiceStyle::CozmoProcessing;
      pitchRange.SetRange(-0.1f, 0.1f);
      stepSize = 0.1f;
      isRandom = true;
      if (text.length() < kCharLengthThreshold) {
        // Short names
        durationRange.SetRange(2.35f, 2.65f);
      }
      else {
        // Long names
        durationRange.SetRange(2.25f, 2.55f);
      }
      break;
    }
      
    case SayTextIntent::Name_FirstIntroduction_2:
    {
      // Say name slightly faster then the first time
      _style = SayTextVoiceStyle::CozmoProcessing;
      pitchRange.SetRange(0.3f, 0.6f);
      stepSize = 0.1f;
      isRandom = true;
      if (text.length() < kCharLengthThreshold) {
        // Short names
        durationRange.SetRange(2.f, 2.2f);
      }
      else {
        // Long names
        durationRange.SetRange(1.9f, 2.1f);
      }
      break;
    }
    
    case SayTextIntent::Count:
      ASSERT_NAMED(SayTextIntent::Count == intent, "SayTextAction.ActionStyle.SayTextActionStyle::Count.IsInvalid");
      break;
  }
  
  // If random create
  if (isRandom) {
    // Break duration scalar range into step sizes
    ASSERT_NAMED(Util::IsFltGTZero(stepSize), "SayTextAction.SayTextAction.stepSize.IsZero");
    // (Scalar Range / stepSize) + 1 = number of total possible steps
    const uint8_t stepCount = ((durationRange.max - durationRange.min) / stepSize) + 1;
    const auto randStep = robot.GetRNG().RandInt(stepCount);
    _durationScalar = durationRange.min + (stepSize * randStep);
    
    // Set Random pitch
    _voicePitch = Util::numeric_cast<float>(robot.GetRNG().RandDblInRange(pitchRange.min, pitchRange.max));
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
        using namespace Audio;
        const GameEvent::GenericEvent audioEvent = _robot.GetTextToSpeechComponent().GetAudioEvent(_style);
        _animation.SetIsLive(true);
        _animation.AddKeyFrameToBack(RobotAudioKeyFrame(RobotAudioKeyFrame::AudioRef(audioEvent), 0));
        _playAnimationAction = new PlayAnimationAction(_robot, &_animation);
        
        // Set voice Pitch
        // FIXME: This is a temp fix, we are asuming the TtS animatoin is using the CozmoBus_1 or Cozmo_OnDevice game object.
        // We need to add the ability to set rtpc in animations so they are posted with the animation audio events.
        GameObjectType gameObj = (_robot.GetRobotAudioClient()->GetOutputSource() == RobotAudioClient::RobotAudioOutputSource::PlayOnRobot) ?
                                  GameObjectType::CozmoBus_1 : GameObjectType::Cozmo_OnDevice;
        _robot.GetRobotAudioClient()->PostParameter(Audio::GameParameter::ParameterType::External_Process_Pitch,
                                                    _voicePitch,
                                                    gameObj);
      }
      else {
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
