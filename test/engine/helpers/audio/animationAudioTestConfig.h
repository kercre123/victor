/**
 * File: helpers/audio/animationAudioTestConfig.h
 *
 * Author: Jordan Rivas
 * Created: 6/17/16
 *
 * Description: Configure an audio animation scenario by inserting each audio event. Generate Animation &
 *              RobotAudioBuffer for given config.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Test_Helpers_Audio_AnimationAudioTestConfig_H__
#define __Test_Helpers_Audio_AnimationAudioTestConfig_H__


#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"

#include <vector>


namespace Anki {
namespace Vector {
namespace RobotAnimation {
class StreamingAnimationTest;
}
namespace Audio {
class RobotAudioTestBuffer;
}
}
}


class AnimationAudioTestConfig {
  
public:
  
  // Define an Audio Event
  struct TestAudioEvent {
    
    Anki::AudioMetaData::GameEvent::GenericEvent event = Anki::AudioMetaData::GameEvent::GenericEvent::Invalid;
    uint32_t startTime_ms = 0;
    uint32_t duration_ms = 0;
    TestAudioEvent( Anki::AudioMetaData::GameEvent::GenericEvent event, uint32_t startTime_ms, uint32_t duration_ms )
    : event( event )
    , startTime_ms( startTime_ms )
    , duration_ms( duration_ms )
    {}
    
    bool InAnimationTimeRange( uint32_t animationTime_ms )
    {
      return ( animationTime_ms >= startTime_ms ) && ( animationTime_ms < GetCompletionTime_ms() );
    }
    
    uint32_t GetCompletionTime_ms() const { return startTime_ms + duration_ms; }
    
    // TODO: Add methods to offset actual buffer creation to simulate the buffer not perfectly lining up with desired
    //       situation.
  };
  
  // Add Test Event
  // Note: Can not call this after Insert Complete has been called
  void Insert( TestAudioEvent&& audioEvent );

  // Call to lock insert and setup meta data
  void InsertComplete();
  
  // Get list of sorted events that start within frame time
  std::vector<TestAudioEvent> FrameAudioEvents( const int32_t frameStartTime_ms, const int32_t frameEndTime_ms );
  
  std::vector<TestAudioEvent> GetCurrentPlayingEvents( const int32_t frameStartTime_ms, const int32_t frameEndTime_ms, const uint32_t framedrift_ms );
  
  // Write audio into Test Robot Buffer
  const std::vector<TestAudioEvent>& GetAudioEvents() const { return _events; }
  
  // Add Config's events to animation
  void LoadAudioKeyFrames( Anki::Vector::Animation& outAnimation );
  
  // Create or write audio into Test Robot Buffer
  void LoadAudioBuffer( Anki::Vector::Audio::RobotAudioTestBuffer& outBuffer );
  
  // Add Events and fake audio buffer data to Streaming Animation
  void LoadStreamingAnimation( Anki::Vector::RobotAnimation::StreamingAnimationTest& out_streamingAnimation );
  
  
private:
  
  std::vector<TestAudioEvent> _events;
  bool _lockInsert = false;

  bool IsNextEventReady(size_t idx, uint32_t animationTime_ms);
  
};


#endif /* __Test_Helpers_Audio_AnimationAudioTestConfig_H__ */
