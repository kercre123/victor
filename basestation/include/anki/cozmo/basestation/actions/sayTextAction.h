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

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "clad/types/sayTextStyles.h"
#include "clad/types/animationTrigger.h"
#include <limits>


namespace Anki {
namespace Util {
namespace Data {
class DataPlatform;
}
}
namespace Cozmo {

class SayTextAction : public IAction
{
public:
  
  // Load Static Cozmo Say Text Intent metadata from disk
  // Return true on success
  static bool LoadMetadata(Util::Data::DataPlatform& dataPlatform);
  
  // Customize the text to speech creation by setting the voice style and duration scalar.
  // Note: The duration scalar stretches the duration of the generated TtS audio. When using the unprocessed voice
  //       you can use a values around 1.0 which is what the TtS generator normal speed. When using CozmoProcessing
  //       it is more common to use a value between 1.8 - 2.3 which gets sped up in the audio engine resulting in a
  //       duration close to the unprocessed voice.
  //       When using SayTextVoiceStyle::CozmoProcessing adjust processing pitch by seeting value [-1.0, 1.0].
  SayTextAction(Robot& robot, const std::string& text,
                const SayTextVoiceStyle style,
                const float durationScalar = 1.f,
                const float voicePitch = 0.f);
  
  // Play a predefined text to speech style by passing in an intent
  SayTextAction(Robot& robot, const std::string& text, const SayTextIntent intent);
  
  virtual ~SayTextAction();
  
  virtual f32 GetTimeoutInSeconds() const override { return _timeout_sec; }
  
  // Use a the animation group tied to a specific GameEvent.
  // Use AnimationTrigger::Count to use built-in animation (default).
  // The animation group should contain animations that have the special audio keyframe for
  // Audio::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing or Play__Robot_Vo__External_Unprocessed
  void SetAnimationTrigger(AnimationTrigger trigger, u8 ignoreTracks = 0);
  
  // Generate new animation by stiching the animation group animations together until they are equal or greater to the
  // duration of generated text to speech conent
  // Note: Animation Trigger must not have Play__Robot_Vo__External_Cozmo_Processing audio event in the
  // animation. The event will be added to the first frame when generating the animation to fit the duration.
  void SetFitToDuration(bool fitToDuration) { _fitToDuration = fitToDuration; }

  
protected:

  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;


private:
  
  std::string               _text;
  SayTextVoiceStyle         _style;
  float                     _durationScalar       = 0.f;
  float                     _voicePitch           = 0.f;      // Adjust Cozmo voice processing pitch [-1.0, 1.0]
  uint8_t                   _ttsOperationId       = 0;        // This is set while the action is managing the audio data
  bool                      _isAudioReady         = false;
  Animation                 _animation;
  AnimationTrigger          _animationTrigger     = AnimationTrigger::Count; // Count == use built-in animation
  u8                        _ignoreAnimTracks     = (u8)AnimTrackFlag::NO_TRACKS;
  IActionRunner*            _playAnimationAction  = nullptr;
  bool                      _fitToDuration        = false;
  f32                       _timeout_sec          = 30.f;
  
  // Call to start processing text to speech
  void GenerateTtsAudio();
  
  // Append animation by stitching animation trigger group animations together until the animation's duration is
  // greater or equal to the provided duration
  void UpdateAnimationToFitDuration(const float duration_ms);
  
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
      
      ConfigTrait();
      ConfigTrait(const Json::Value& json);
      
      float GetDuration(Util::RandomGenerator& randomGen) const;
    };
    
    std::string name = "";
    SayTextVoiceStyle style = SayTextVoiceStyle::CozmoProcessing_Sentence;
    std::vector<ConfigTrait> durationTraits;
    std::vector<ConfigTrait> pitchTraits;
    
    SayTextIntentConfig();
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
