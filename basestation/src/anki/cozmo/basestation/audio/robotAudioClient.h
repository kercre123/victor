/*
 * File: robotAudioClient.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Client handles the Robot’s specific audio needs. It is a sub-class of AudioEngineClient.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_RobotAudioClient_H__
#define __Basestation_Audio_RobotAudioClient_H__

#include "anki/cozmo/basestation/audio/audioEngineClient.h"
#include "anki/cozmo/basestation/animations/track.h"
#include <util/dispatchQueue/dispatchQueue.h>
#include <list>
#include <unordered_map>


namespace Anki {
namespace Cozmo {

class Animation;

namespace AnimKeyFrame {
struct AudioSample;
}

  
namespace Audio {
  
class RobotAudioBuffer;
class RobotAudioBufferStream;
  
class RobotAudioClient : public AudioEngineClient
{
public:

  RobotAudioClient();
  ~RobotAudioClient();
  
  void SetAudioBuffer( RobotAudioBuffer* audioBuffer ) { _audioBuffer = audioBuffer; }
  
  // Post Cozmo specific Audio events
  CallbackIdType PostCozmoEvent( EventType event,
                                 AudioCallbackFlag callbackFlag = AudioCallbackFlag::EventNone );
  
  // Load animation and begin to buffer audio
  bool LoadAnimationAudio( Animation* anAnimation );
  
  // This returns true after LoadAnimationAudio() is performed and remains true until the last audio key frame
  // is completed
  bool IsPlayingAnimation() const { return _isPlayingAnimation; }
  
  // Current Animation id
  // Return empty string if no animation is playing
  const std::string& GetCurrentAnimationName() const { return _animationName; }
  
  // TODO: This currently just clears all metadata and audio buffered data. It is not pleasant to the ears.
  // This is called after the last audio key frame has been popped
  void ClearAnimation();
  
  // Return false if we expect to have buffer however it is not ready yet
  bool PrepareAudioKeyFrame( TimeStamp_t startTime_ms,
                             TimeStamp_t streamingTime_ms);

  // Pop the front key frame audio sample message
  // Will set out_AudioSamplePtr to Null if there are no key frames for provided current time. Use this to identify when
  // to send a AudioSilence key frame.
  // Note: Audio Sample pointer memory needs to be manage or it will leak memory.
  // Return false if we expect to have buffer however it is not ready yet
  bool PopAudioKeyFrame( AnimKeyFrame::AudioSample*& out_AudioSamplePtr,
                         TimeStamp_t startTime_ms,
                         TimeStamp_t streamingTime_ms );
  
  // Set the minimum amount of Audio Key frames that should be buffered before return true "ready" from
  // PrepareAudioKeyFrame() or PopAudioKeyFrame()
  void SetPreBufferKeyFrameCount( uint8_t keyFrameCount ) { _PreBufferKeyFrameCount = keyFrameCount; }
  
  // Check if there is enough initial audio buffer to begin streaming animation
  // This returns true if there is no RobotAudioBuffer
  bool IsFirstBufferReady();

protected:
  
  // Override callback events for client
  virtual void HandleCallbackEvent( const AudioCallbackDuration& callbackMsg ) override;
  virtual void HandleCallbackEvent( const AudioCallbackMarker& callbackMsg ) override;
  virtual void HandleCallbackEvent( const AudioCallbackComplete& callbackMsg ) override;
  
  
private:
  
  // Provides cozmo audio data
  RobotAudioBuffer* _audioBuffer = nullptr;
  
  // Dispatch event
  Util::Dispatch::Queue* _postEventTimerQueue;
  
  // Amount of audio key frames that need to be buffered before starting animation
  uint8_t _PreBufferKeyFrameCount = 0;
  
  // Struct to sync audio buffer streams with animation
  struct AnimationEvent {
    EventType AudioEvent;
    uint32_t TimeInMS;
    AnimationEvent( EventType audioEvent, uint32_t timeInMS ) :
    AudioEvent( audioEvent ),
    TimeInMS( timeInMS ) {}
  };
  // Ordered list of animation audio event
  std::list<AnimationEvent> _animationEventList;

  
  // Animation properties
  // State Flags
  bool _isPlayingAnimation = false;
  bool _isFirstBufferLoaded = false;
  // Animation id
  std::string _animationName = "";
  // Track Queue's front stream
  RobotAudioBufferStream* _currentBufferStream = nullptr;
  
  // Audio buffer time shift, this allow us to buffer audio as soon as the animation is loaded and play
  // events relevant to each other.
  uint32_t _firstAudioEventOffset = 0;
  
};
  
} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_RobotAudioClient_H__ */
