/**
* File: RobotAudioTestClient
*
* Author: damjan stulic
* Created: 6/9/16
*
* Description: This Client handles the Robot’s specific audio needs. It is a sub-class of AudioEngineClient.
*
* Copyright: Anki, inc. 2016
 */

#ifndef __Test_Helpers_Audio_RobotAudioTestClient_H__
#define __Test_Helpers_Audio_RobotAudioTestClient_H__

#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "helpers/audio/robotAudioTestBuffer.h"


namespace Anki {
namespace Cozmo {
namespace Audio {

class RobotAudioTestClient : public RobotAudioClient
{
public:

  // Default Constructor
  RobotAudioTestClient( );

  // The the audio buffer for the corresponding Game Object
  RobotAudioBuffer* GetRobotAudiobuffer( AudioMetaData::GameObjectType gameObject ) override;

private:
  RobotAudioTestBuffer _robotAudioTestBuffer;
};

} // Audio
} // Cozmo
} // Anki



#endif /* __Test_Helpers_Audio_RobotAudioTestClient_H__ */
