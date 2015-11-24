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
  
  // True if Plug-In is streaming audio data
  bool IsPlugInActive() const;
  
  // True if key frame audio sample messages are available
  bool HasKeyFrameAudioSample() const;
  
  // Pop the front key frame audio sample message
  AnimKeyFrame::AudioSample&& PopAudioSample() const;

  
protected:
  
  virtual void HandleCallbackEvent( const AudioCallbackDuration& callbackMsg ) override;
  virtual void HandleCallbackEvent( const AudioCallbackMarker& callbackMsg ) override;
  virtual void HandleCallbackEvent( const AudioCallbackComplete& callbackMsg ) override;
  
private:
  
  std::unordered_map<CallbackIdType, EventType> _currentEvents;
  
  RobotAudioBuffer& _audioBuffer;
  
};
  
} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_RobotAudioClient_H__ */
