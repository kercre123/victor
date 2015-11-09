//
//  robotAudioClient.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/8/15.
//
//

#ifndef robotAudioClient_h
#define robotAudioClient_h

#include "anki/cozmo/basestation/audio/audioEngineClient.h"

namespace Anki {
namespace Cozmo {
namespace Audio {
  
class RobotAudioClient : public AudioEngineClient
{
public:
  
  RobotAudioClient( AudioEngineMessageHandler* messageHandler );
  
};
  
} // Audio
} // Cozmo
} // Anki



#endif /* robotAudioClient_h */
