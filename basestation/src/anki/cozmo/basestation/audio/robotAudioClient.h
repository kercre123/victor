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
  
  bool GetSoundSample(const uint32_t sampleIdx, AnimKeyFrame::AudioSample &msg);
  
private:
  
  RobotAudioBuffer& _audioBuffer;
  
};
  
} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_RobotAudioClient_H__ */
