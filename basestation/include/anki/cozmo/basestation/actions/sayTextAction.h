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


namespace Anki {
namespace Cozmo {

class SayTextAction : public IAction
{
public:
  
  // Customize the text to speech creation by setting the voice style and duration scalar.
  // Note: The duration scalar stretches the duration of the generated TtS audio. When using the unprocessed voice
  //       you can use a values around 1.0 which is what the TtS generator normal speed. When using CozmoProcessing
  //       it is more common to use a value between 1.8 - 2.3 which gets sped up in the audio engine resulting in a
  //       duration close to the unprocessed voice.
  SayTextAction(Robot& robot, const std::string& text, const SayTextVoiceStyle style, const float durationScalar);
  
  // Play a predefined text to speech style by passing in an intent
  SayTextAction(Robot& robot, const std::string& text, const SayTextIntent intent);
  
  virtual ~SayTextAction();
  
  virtual f32 GetTimeoutInSeconds() const override { return _timeout_sec; }
  
  // Use a the animation group tied to a specific GameEvent.
  // Use AnimationTrigger::Count to use built-in animation (default).
  // The animation group should contain animations that have the special audio keyframe for
  // Audio::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing or Play__Robot_Vo__External_Unprocessed
  void SetAnimationTrigger(AnimationTrigger trigger) { _animationTrigger = trigger; }
  
protected:
  
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;

private:
  
  std::string               _text;
  SayTextVoiceStyle         _style;
  float                     _durationScalar       = 0.f;
  uint8_t                   _ttsOperationId       = 0;        // This is set while the action is managing the audio data
  bool                      _isAudioReady         = false;
  Animation                 _animation;
  AnimationTrigger          _animationTrigger     = AnimationTrigger::Count; // Count == use built-in animation
  IActionRunner*            _playAnimationAction  = nullptr;
  f32                       _timeout_sec          = 30.f;
  
  // Call to start processing text to speech
  void GenerateTtsAudio();
  
}; // class SayTextAction


} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Actions_SayTextAction_H__ */
