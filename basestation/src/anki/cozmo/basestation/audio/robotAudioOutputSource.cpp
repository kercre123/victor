//
//  robotAudioOutputSource.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 5/17/16.
//
//


#include "anki/cozmo/basestation/audio/mixing/audioMixingConsole.h"
#include "anki/cozmo/basestation/audio/robotAudioOutputSource.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/animationKeyFrames.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"


constexpr double INT16_MAX_DBL = double(INT16_MAX);


namespace Anki {
namespace Cozmo {
namespace Audio {
  

RobotAudioOutputSource::RobotAudioOutputSource( const AudioMixingConsole& mixingConsole )
: AudioMixerOutputSource( mixingConsole )
{
  ASSERT_NAMED( _mixingConsole.GetFrameSize() == static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ), "RobotAudioOutputSource.AudioFrameSampleSizeNotEqual");
}

// Final mix, apply output processing and provide data to destination
void RobotAudioOutputSource::ProcessTick( const MixingConsoleBuffer& audioFrame )
{
  ASSERT_NAMED(_robotAudioFrameMsg == nullptr, "RobotAudioOutputSource.ProcessTick._robotAudioFrameMsg.NotNull");
  if ( audioFrame == nullptr || _mute ) {
    // Nothing to proces
//    if ( _robotAudioFrameMsg != nullptr ) {
//      // Didn't pop frame last tick
//      PRINT_NAMED_INFO("RobotAudioOutputSource.ProcessTick", "Unused robot audio frame message!");
//      Util::SafeDelete( _robotAudioFrameMsg );
//    }
    
    _robotAudioFrameMsg = new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence());
    
    return;
  }
  
  // Create Audio message
  AnimKeyFrame::AudioSample keyFrame;
  uint8_t* destination = keyFrame.sample.data();
  encodeMuLaw( audioFrame, destination, _mixingConsole.GetFrameSize(), _volume );

  // Pad the back of the buffer with 0s
  // This should only apply to the last frame
//  if (audioFrame->sampleCount < static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE )) {
//    std::fill( keyFrame.sample.begin() + audioFrame->sampleCount, keyFrame.sample.end(), 0 );
//  }
  
  _robotAudioFrameMsg = new RobotInterface::EngineToRobot( std::move( keyFrame ) );
}


const RobotInterface::EngineToRobot* RobotAudioOutputSource::PopRobotAudioFrameMsg()
{
  // No longer own frame massage
  const auto tempMsg = _robotAudioFrameMsg;
  _robotAudioFrameMsg = nullptr;
  return tempMsg;
}


  
  
bool RobotAudioOutputSource::encodeMuLaw( const MixingConsoleBuffer& in_samples, uint8_t*& out_samples, size_t size, float volumeScalar )
{
  
//  std::string outputStr;
//  for (size_t idx = 0; idx < size; ++idx) {
//    outputStr += std::to_string(in_samples[idx]) + ", ";
//  }
//  PRINT_NAMED_WARNING("RobotAudioOutputSource.encodeMuLaw", "Vol: %f - stream: %s", volumeScalar, outputStr.c_str());
  
  int16_t sample;
  bool sign;
  uint8_t exponent, mantessa;
  double vol = volumeScalar;
  for ( size_t idx = 0; idx < size; ++idx ) {
    
    // Reformat audio frame
    static const uint8_t MuLawCompressTable[] =
    {
      0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
      5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
      6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
      6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
      7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
      7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
      7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
      7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
    };
    
    // Convert float (-1.0, 1.0) to int16
    sample = Util::numeric_cast<int16_t>( in_samples[idx] * vol * INT16_MAX_DBL );
    
    sign = sample < 0;
    
    if (sign)	{
      sample = ~sample;
    }
    
    exponent = MuLawCompressTable[sample >> 8];
    
    if (exponent) {
      mantessa = (sample >> (exponent + 3)) & 0xF;
    } else {
      mantessa = sample >> 4;
    }
    
    out_samples[idx] = Util::numeric_cast<uint8_t>( (sign ? 0x80 : 0) | (exponent << 4) | mantessa );
  }
  
  return true;
}

} // Audio
} // Cozmo
} // Anki
