//
//  engineAudioInput.hpp
//  cozmoEngine2
//
//  Created by Jordan Rivas on 9/12/17.
//
//

#ifndef engineAudioInput_hpp
#define engineAudioInput_hpp


#include "audioEngine/multiplexer/audioMuxInput.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"


namespace Anki {
namespace Cozmo {
namespace Audio {

class EngineAudioInput : public AudioEngine::Multiplexer::AudioMuxInput {
  
public:
  
  void HandleEngineToRobotMsg(RobotInterface::EngineToRobot& msg);
  
  virtual void PostCallback( const AudioEngine::Multiplexer::AudioCallbackDuration&& callbackMessage ) const override;
  virtual void PostCallback( const AudioEngine::Multiplexer::AudioCallbackMarker&& callbackMessage ) const override;
  virtual void PostCallback( const AudioEngine::Multiplexer::AudioCallbackComplete&& callbackMessage ) const override;
  virtual void PostCallback( const AudioEngine::Multiplexer::AudioCallbackError&& callbackMessage ) const override;
  
};


} // Audio
} // Cozmo
} // Anki




#endif /* engineAudioInput_hpp */
