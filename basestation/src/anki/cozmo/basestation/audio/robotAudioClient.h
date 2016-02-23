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
#include <mutex>


namespace Anki {
namespace Cozmo {

class Animation;
  
namespace Audio {
  
class RobotAudioBuffer;
class RobotAudioMessageStream;
  
class RobotAudioClient : public AudioEngineClient
{
public:

  RobotAudioClient();
  ~RobotAudioClient();
  
  void SetAudioBuffer( RobotAudioBuffer* audioBuffer ) { _audioBuffer = audioBuffer; }
  
  // Post Cozmo specific Audio events
  CallbackIdType PostCozmoEvent( GameEvent::GenericEvent event, AudioEngineClient::CallbackFunc callback = nullptr );
  
  // Load animation and begin to buffer audio
  bool LoadAnimationAudio( Animation* anAnimation );
  
  // This returns true after LoadAnimationAudio() is performed and remains true until the last audio message
  // is completed
  bool IsPlayingAnimation() const { return _isPlayingAnimation; }
  
  // Current Animation id
  // Return empty string if no animation is playing
  const std::string& GetCurrentAnimationName() const { return _animationName; }
  
  // TODO: This currently just clears all metadata and audio buffered data. It is not pleasant to the ears.
  // This is called after the last audio messages has been proccessed or animation is aborted
  void ClearAnimation();
  
  // Return false if we expect to have buffer however it is not ready yet
  bool PrepareRobotAudioMessage( TimeStamp_t startTime_ms,
                                 TimeStamp_t streamingTime_ms );

  // Pop the front EngineToRobot audio message
  // Will set out_RobotAudioMessagePtr to Null if there are no messages for provided current time. Use this to identify
  // when to send a AudioSilence message.
  // Note: EngineToRobot pointer memory needs to be manage or it will leak memory.
  // Return false if we expect to have buffer however it is not ready yet
  bool PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                             TimeStamp_t startTime_ms,
                             TimeStamp_t streamingTime_ms );
  
  // Set the minimum amount of Audio messages that should be buffered before return true "ready" from
  // PrepareRobotAudioMessage() or PopRobotAudioMessage()
  void SetPreBufferRobotAudioMessageCount( uint8_t count ) { _PreBufferRobotAudioMessageCount = count; }
  
  // Check if there is enough initial audio buffer to begin streaming animation
  // This returns true if there is no RobotAudioBuffer
  bool IsFirstBufferReady();

  
private:
  
  // Provides cozmo audio data
  RobotAudioBuffer* _audioBuffer = nullptr;
  
  // Dispatch event
  Util::Dispatch::Queue* _postEventTimerQueue;
  
  // Amount of audio messages that need to be buffered before starting animation
  uint8_t _PreBufferRobotAudioMessageCount = 0;
  
  // Struct to sync audio buffer streams with animation
  struct AnimationEvent {
    using AnimationEventId = uint16_t;
    static constexpr AnimationEventId kInvalidAnimationEventId = 0;
    
    AnimationEventId EventId = kInvalidAnimationEventId;
    GameEvent::GenericEvent AudioEvent = GameEvent::GenericEvent::Invalid;
    uint32_t TimeInMS = 0;
    AnimationEvent( AnimationEventId eventId, GameEvent::GenericEvent audioEvent, uint32_t timeInMS ) :
      EventId( eventId ),
      AudioEvent( audioEvent ),
      TimeInMS( timeInMS ) {}
  };
  // Ordered list of animation audio event
  std::list<AnimationEvent> _animationEventList;
  // List of failed animation events
  std::list<AnimationEvent::AnimationEventId> _invalidAnimationIds;
  // Guard against event errors on different threads
  std::mutex _invalidAnimationIdLock;

  // Animation properties
  // State Flags
  bool _isPlayingAnimation = false;
  bool _isFirstBufferLoaded = false;
  // Animation id
  std::string _animationName = "";
  // Track Queue's front stream
  RobotAudioMessageStream* _currentBufferStream = nullptr;
  
  // Audio buffer time shift, this allow us to buffer audio as soon as the animation is loaded and play
  // events relevant to each other.
  uint32_t _firstAudioEventOffset = 0;
  
  // Handle Cozmo event callbacks, specifically errors
  void HandleCozmoEventCallback( AnimationEvent::AnimationEventId eventId, AudioCallback& callback );
  
};
  
} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_RobotAudioClient_H__ */
