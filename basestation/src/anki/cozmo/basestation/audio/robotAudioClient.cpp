/*
 * File: robotAudioClient.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Client handles the Robot’s specific audio needs. It is a sub-class of AudioEngineClient.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/cozmo/basestation/keyframe.h"

#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
//static const uint32_t SOUND_SAMPLE_SIZE = 800;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioClient::RobotAudioClient( AudioEngineMessageHandler& messageHandler, RobotAudioBuffer& audioBuffer ) :
  AudioEngineClient( messageHandler ),
  _audioBuffer( audioBuffer )
{

}
  
bool RobotAudioClient::GetSoundSample(const uint32_t sampleIdx, AnimKeyFrame::AudioSample &msg)
{
  
  // TODO: Do stuff!
  
  _audioBuffer.hasBuffer();
  return true;
}

} // Audio
} // Cozmo
} // Anki
