//
//  robotAudioClient.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/8/15.
//
//

#include "anki/cozmo/basestation/audio/robotAudioClient.h"

#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioClient::RobotAudioClient( AudioEngineMessageHandler* messageHandler ) :
  AudioEngineClient( messageHandler)  
{
  
}

} // Audio
} // Cozmo
} // Anki
