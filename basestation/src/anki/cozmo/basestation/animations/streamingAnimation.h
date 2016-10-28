/*
 * File: streamingAnimation.h
 *
 * Author: Jordan Rivas
 * Created: 07/19/16
 *
 * Description:
 *
 *  NOTE: This class copies parent class Animation and buffers audio
 *        This class must stay in memory until all of the frames have been turned into Robot Messages
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Animations_StreamingAnimation_H__
#define __Basestation_Animations_StreamingAnimation_H__

#include "anki/cozmo/basestation/animation/animation.h"
#include "anki/cozmo/basestation/animations/animationControllerTypes_Internal.h"
#include "clad/audio/audioEventTypes.h"

#include "clad/audio/audioGameObjectTypes.h"


#include <list>
#include <mutex>


namespace AudioEngine {
struct AudioCallbackInfo;
}
namespace Anki {
namespace Util {
class RandomGenerator;
}
namespace Cozmo {
namespace Audio {
struct AudioFrameData;
class RobotAudioBuffer;
class RobotAudioClient;
class RobotAudioFrameStream;
}

namespace RobotAnimation {
 
  
class StreamingAnimation : public Animation  {
  
public:
  
  enum class BufferState {
    None = 0,       // No Audio data, yet
    WaitBuffering,  // Waiting for audio data
    ReadyBuffering, // Has audio data but is not done buffering
    Completed       // Has all audio data for animation
  };
  
  // Init a Sreaming Aunimation by copying a parrent Animation
  StreamingAnimation(const Animation& origalAnimation);
  
  // Reset Animation Playhead to beginning
  Result Init();
  
  BufferState GetBufferState() const { return _state; }
  
  // Return true if the animation's audio has random events, therefore only playonce and throw away
  bool GetHasRandomAudioEvents() const { return _hasRandomEvents; }
  
  // Return true if an animtaion audio event event has alternat audio when it is played
  bool GetHasAlternateEventAudio() const { return _hasAltEventAudio; }
  
  // Check if the next audio frame is ready
  bool CanPlayNextFrame() const;
  
  // Update State & Audio data
  void Update();

  
  // Return false if there are no audio data frames to tick frame, check state to deturmine why (either still buffering
  // or complete)
  // Note: Next frame must be ready before ticking
  void TickPlayhead(KeyframeList& out_keyframeList, const Audio::AudioFrameData*& out_audioFrame);
  
  // Once started must be ticked with every frame until completed
  bool DidStartPlaying() const { return _playheadTime_ms > 0; }
  
  bool IsPlaybackComplete() const { return _state == BufferState::Completed &&
                                           _playheadFrame == _audioBufferedFrameCount; }
  
  // Returns the known value of frames left or UINT32_MAX to identify undknow because audio frames are still buffering
  static constexpr uint32_t kUnknownFramesLeft = UINT32_MAX;
  uint32_t FramesLeft() const { return _state == BufferState::Completed ?
                                      (_audioBufferedFrameCount - _playheadFrame) : kUnknownFramesLeft; }
  
  // Get Playback Info
  uint32_t GetPlayheadFrame() const { return _playheadFrame; }
  uint32_t GetPlayheadTime_ms() const { return _playheadTime_ms; }
  
  
  // Use animation audio event keyframes to generate a list of playable audio events
  void GenerateAudioEventList(Util::RandomGenerator& randomGenerator);
  
  // Begin to buffer Animation's audio data
  // Note: Besure the audioClient has an available buffer in it's pool before calling
  void GenerateAudioData(Audio::RobotAudioClient* audioClient);
  
  // This creates the events needed to play synchronous cozmo audio on device
  // TODO: Need to implement this
  void GenerateDeviceAudioEvents(Audio::RobotAudioClient* audioClient);

  
private:
  
  // Struct to sync audio buffer streams with animation
  struct AnimationEvent {
    using AnimationEventId = uint16_t;
    static constexpr AnimationEventId kInvalidAnimationEventId = 0;
    enum class AnimationEventState {
      None = 0,     // Not Set
      Posted,       // Posted audio event
      Completed,    // Completed event
      Error         // Event had play error
    };
    
    AnimationEventId eventId = kInvalidAnimationEventId;
    Audio::GameEvent::GenericEvent audioEvent = Audio::GameEvent::GenericEvent::Invalid;
    uint32_t time_ms = 0;
    float volume = 1.0f;
    AnimationEventState state = AnimationEventState::None;
    
    
    AnimationEvent(AnimationEventId eventId, Audio::GameEvent::GenericEvent audioEvent, uint32_t time_ms, float volume = 0.0f)
    : eventId(eventId)
    , audioEvent(audioEvent)
    , time_ms(time_ms)
    , volume(volume) {}
  };
  
  // Provide week pointers to help with callbacks
  std::shared_ptr<char> _isAliveSharedPtr = std::make_shared<char>();
  
  // Streaming Animation State
  BufferState _state = BufferState::None;
  
  // Audio variety flags
  // If either is true the animation should only be used once to create a more unique experience
  bool _hasRandomEvents = false;
  bool _hasAltEventAudio = false;
  
  // Playing and buffering audio objects
  Audio::GameObjectType     _gameObj = Audio::GameObjectType::Invalid;
  Audio::RobotAudioClient*  _audioClient = nullptr;
  Audio::RobotAudioBuffer*  _audioBuffer = nullptr;
  
  // Hold on to the current stream frames are being pulled from
  Audio::RobotAudioFrameStream* _currentBufferStream = nullptr;
  bool HasCurrentBufferStream() const { return _currentBufferStream != nullptr; }
  
  // Add audio frame data to animation
  // NOTE: Takes owner ship of frame
  void PushFrameIntoBuffer(const Audio::AudioFrameData* frame);
  using AudioFrameList = std::list<const Audio::AudioFrameData*>;
  AudioFrameList            _audioFrames;
  
  // Buffer & Playback Info
  uint32_t _lastKeyframeTime_ms = 0;
  uint32_t _audioBufferedFrameCount = 0;
  uint32_t _playheadFrame = 0;
  uint32_t _playheadTime_ms = 0;
  AudioFrameList::iterator  _audioPlayheadIt;
  
  // Guard against event errors on different threads
  std::mutex _animationEventLock;
  
  // List of playable audio events
  using AudioEventList = std::vector<AnimationEvent>;
  AudioEventList _animationAudioEvents;
  
  // Track the time the first stream was created and audio event to calculate the streams relevant animation time
  const double kInvalidAudioSreamOffsetTime = std::numeric_limits<double>::min();
  double _audioStreamOffsetTime_ms = kInvalidAudioSreamOffsetTime;

  // Get the next audio event
  // Note: Will return null when there are no more events left
  const AnimationEvent* GetNextEvent() const;
  
  // Track the current Event in the _animationAudioEvents vector
  // Note: This is not thread safe, be sure to properly lock when using
  int GetEventIndex() const { return _eventIndex; }
  void IncrementEventIndex() { ++_eventIndex; }
  int _eventIndex = 0;
  
  // Track number of events that have been requested to play
  // Note: This is not thread safe, be sure to properly lock when using
  int GetPostedEventCount() const { return _postedEventCount; }
  void IncrementPostedEventCount() { ++_postedEventCount; }
  int _postedEventCount = 0;
  
  // Track how many events have completed playback to track animation completion state
  // Note: This is not thread safe, be sure to properly lock when using
  int GetCompletedEventCount() const { return _completedEventCount; }
  void IncrementCompletedEventCount() { ++_completedEventCount; }
  int _completedEventCount = 0;
  
  // Handle Cozmo Event callbacks from Audio Engine
  void HandleCozmoEventCallback(AnimationEvent* animationEvent, const AudioEngine::AudioCallbackInfo& callbackInfo);
  
  // Copy audio frames into animation audio frame list
  void CopyAudioBuffer();
  
  // This is a helper method to CopyAudioBuffer()
  void AddAudioSilenceFrames();
  void AddAudioDataFrames();
  
  // Audio buffer with silence
  void AddFakeAudio();
  
  // Check if all of the audio events have been cached and there is an audio frame for every animation frame
  bool IsAnimationDone() const;
  
};

} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki

#endif /* __Basestation_Animations_StreamingAnimation_H__ */
