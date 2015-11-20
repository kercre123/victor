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
#include <unordered_map>


namespace Anki {
namespace Cozmo {
  
namespace AnimKeyFrame {
struct AudioSample;
}
  
namespace Audio {
  
class RobotAudioBuffer;
  
class RobotAudioClient : public AudioEngineClient
{
public:

  RobotAudioClient( AudioEngineMessageHandler& messageHandler, RobotAudioBuffer& audioBuffer );
  
  // Post Cozmo specific Audio events
  CallbackIdType PostCozmoEvent( EventType event,
                                 AudioCallbackFlag callbackFlag = AudioCallbackFlag::EventNone );

  // Get audio samples from buffer
  // Return: Number of samples returned
  uint32_t GetSoundSample( uint8_t* outArray, uint32_t requestedSize );

  // Return true when when pre buffer is ready
  bool IsReadyToStartAudioStream();
  
  void SetPreBufferSize( uint32_t bufferSize ) { _preBufferSize = bufferSize; }
  
  uint32_t GetBufferSize() const;
  
  bool IsStreaming() const { return _isStreaming; }

private:
  
  std::unordered_map<CallbackIdType, EventType> _currentEvents;
  
  RobotAudioBuffer& _audioBuffer;
  uint32_t _preBufferSize = 0;
  bool _isStreaming = false;
  
  virtual void HandleEvents(const AnkiEvent<MessageAudioClient>& event) override;

};
  
} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_RobotAudioClient_H__ */
