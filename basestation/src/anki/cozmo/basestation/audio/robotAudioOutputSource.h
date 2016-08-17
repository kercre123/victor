//
//  robotAudioOutputSource.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 5/17/16.
//
//

#ifndef __Basestation_Audio_RobotAudioOutputSource_H__
#define __Basestation_Audio_RobotAudioOutputSource_H__

#include "anki/cozmo/basestation/audio/mixing/audioMixerOutputSource.h"
#include "anki/cozmo/basestation/audio/mixing/audioMixingTypes.h"
#include <util/helpers/noncopyable.h>
#include <stdio.h>

namespace Anki {
namespace Cozmo {
namespace RobotInterface {
class EngineToRobot;
}
namespace Audio {
 
class AudioMixingConsole;


class RobotAudioOutputSource : public AudioMixerOutputSource {
  
public:
  
  RobotAudioOutputSource( const AudioMixingConsole& mixingConsole );
  
  // Final mix, apply output processing and provide data to destination
  virtual void ProcessTick( const MixingConsoleBuffer& audioFrame ) override;
  
  
  // Get next Robot Audio Frame Message
//  const RobotInterface::EngineToRobot* GetRobotAudioFrameMsg() const { return _robotAudioFrameMsg; }
  
  // Caller takes ownership, if not memory will leak
  const RobotInterface::EngineToRobot* PopRobotAudioFrameMsg();
  
  
private:
  
  RobotInterface::EngineToRobot* _robotAudioFrameMsg = nullptr;
  
  bool encodeMuLaw( const MixingConsoleBuffer& in_samples, uint8_t*& out_samples, size_t size, float volumeScalar );
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioOutputSource_H__ */
