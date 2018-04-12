/**
 * File: sayTextAction.h
 *
 * Author: Andrew Stein
 * Date:   4/23/2016
 *
 * Updated By: Jordan Rivas 08/18/16
 *
 * Description: Defines an action for having Cozmo "say" a text string.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Cozmo_Actions_SayTextAction_H__
#define __Anki_Cozmo_Actions_SayTextAction_H__

#include "engine/actions/actionInterface.h"
#include "engine/actions/animActions.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/sayTextStyles.h"
#include "clad/types/textToSpeechTypes.h"
#include "util/helpers/templateHelpers.h"
#include "util/signals/simpleSignal_fwd.h"

#include <limits>

// Forward declarations
namespace Anki {
  namespace Util {
    namespace Data {
      class DataPlatform;
    }
  }
}

namespace Anki {
namespace Cozmo {

class SayTextAction : public IAction
{
public:

  // Load Static Cozmo Say Text Intent metadata from disk
  // Return true on success
  static bool LoadMetadata(Util::Data::DataPlatform& dataPlatform);

  // Customize the text to speech creation by setting the voice style and duration scalar.
  // Note: The duration scalar stretches the duration of the generated TtS audio. When using the unprocessed voice
  //       you can use a value around 1.0 which is the TtS generator normal speed. When using CozmoProcessing
  //       it is more common to use a value between 1.8 - 2.3 which gets sped up in the audio engine resulting in a
  //       duration close to the unprocessed voice.
  //       When using SayTextVoiceStyle::CozmoProcessing adjust processing pitch by setting value [-1.0, 1.0].
  SayTextAction(const std::string& text,
                const SayTextVoiceStyle style,
                const float durationScalar = 1.f,
                const float voicePitch = 0.f);

  // Play a predefined text to speech style by passing in an intent
  SayTextAction(const std::string& text, const SayTextIntent intent);

  virtual ~SayTextAction();

  virtual f32 GetTimeoutInSeconds() const override { return _timeout_sec; }

  // Use an animation group tied to a specific GameEvent.
  // Use AnimationTrigger::Count to use built-in animation (default).
  // The animation group should contain animations that have the special audio keyframe for
  // Audio::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing or Play__Robot_Vo__External_Unprocessed
  void SetAnimationTrigger(AnimationTrigger trigger, u8 ignoreTracks = 0);

  // Generate new animation by stitching the animation group animations together until they are equal or greater to the
  // duration of generated text to speech content.
  // Note: Animation Trigger must not have Play__Robot_Vo__External_Cozmo_Processing audio event in the
  // animation. The event will be added to the first frame when generating the animation to fit the duration.
  void SetFitToDuration(bool fitToDuration) { _fitToDuration = fitToDuration; }


protected:

  // IAction interface methods
  virtual void OnRobotSet() override final;
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;

private:

  // TTS parameters
  std::string                    _text;
  SayTextIntent                  _intent = SayTextIntent::Count;
  SayTextVoiceStyle              _style = SayTextVoiceStyle::Count;
  float                          _durationScalar       = 1.f;
  float                          _pitchScalar          = 0.f; // Adjust Cozmo voice processing pitch [-1.0, 1.0]
  bool                           _fitToDuration        = false;

  // Tracks lifetime of TTS utterance for this action
  // ID 0 is reserved for "invalid".
  uint8_t                        _ttsID                = 0;

  // State of this TTS utterance
  TextToSpeechState              _ttsState             = TextToSpeechState::Invalid;

  // Accompanying animation, if any
  AnimationTrigger               _animTrigger          = AnimationTrigger::Count; // Count == use built-in animation
  u8                             _ignoreAnimTracks     = (u8)AnimTrackFlag::NO_TRACKS;
  std::unique_ptr<IActionRunner> _animAction           = nullptr;

  // Bookkeeping
  f32                            _timeout_sec          = 30.f;
  f32                            _expiration_sec       = 0.f;
  Signal::SmartHandle            _signalHandle;

  // VIC-2151: Fit-to-duration not supported on victor
  // Append animation by stitching animation trigger group animations together until the animation's duration is
  // greater or equal to the provided
  // void UpdateAnimationToFitDuration(const float duration_ms);

  // Internal state machine
  ActionResult TransitionToDelivering();
  ActionResult TransitionToPlaying();

  // SayTextVoiceStyle lookup up by name
  using SayTextVoiceStyleMap = std::unordered_map<std::string, SayTextVoiceStyle>;

  // Cozmo Says intent metadata config data struct
  struct SayTextIntentConfig
  {
    struct ConfigTrait
    {
      uint textLengthMin = std::numeric_limits<uint>::min();
      uint textLengthMax = std::numeric_limits<uint>::max();
      float rangeMin = std::numeric_limits<float>::min();;
      float rangeMax = std::numeric_limits<float>::max();;
      float rangeStepSize = 0.0f;

      ConfigTrait() = default;
      ConfigTrait(const Json::Value& json);

      float GetDuration(Util::RandomGenerator& randomGen) const;
    };

    std::string name = "";
    SayTextVoiceStyle style = SayTextVoiceStyle::CozmoProcessing_Sentence;
    std::vector<ConfigTrait> durationTraits;
    std::vector<ConfigTrait> pitchTraits;

    SayTextIntentConfig() = default;
    SayTextIntentConfig(const std::string& intentName, const Json::Value& json, const SayTextVoiceStyleMap& styleMap);

    const ConfigTrait& FindDurationTraitTextLength(uint textLength) const;
    const ConfigTrait& FindPitchTraitTextLength(uint textLength) const;
  };

  // Store loaded Cozmo Says intent configs
  using SayIntentConfigMap = std::unordered_map<SayTextIntent, SayTextIntentConfig, Util::EnumHasher>;
  static SayIntentConfigMap _intentConfigs;

}; // class SayTextAction


} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Actions_SayTextAction_H__ */
