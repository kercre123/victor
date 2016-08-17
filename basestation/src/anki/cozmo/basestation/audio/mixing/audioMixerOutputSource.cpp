/*
 * File: audioMixerOutputSource.cpp
 *
 * Author: Jordan Rivas
 * Created: 05/17/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#include "anki/cozmo/basestation/audio/mixing/audioMixerOutputSource.h"
#include "anki/cozmo/basestation/audio/mixing/audioMixingConsole.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {
namespace Audio {
  

AudioMixerOutputSource::AudioMixerOutputSource( const AudioMixingConsole& mixingConsole )
: _mixingConsole( mixingConsole )
{
}

void AudioMixerOutputSource::ProcessAudioFrame( MixingConsoleBuffer& in_out_sourceFrame )
{
  if ( _mute || Util::IsFltNear( _volume , 0.0f ) ) {
    // Discard frame
    Util::SafeDelete( in_out_sourceFrame );
    in_out_sourceFrame = nullptr;
  }
  else if ( Util::IsFltLT(_volume, 1.0f) ) {
    // Apply volume scalar
    double vol = Util::numeric_cast<double>( _volume );
    for ( size_t idx = 0; idx < _mixingConsole.GetFrameSize(); ++idx ) {
      in_out_sourceFrame[idx] *= in_out_sourceFrame[idx] * vol;
    }
  }
  // Else volume is 1.0 therefore don't do any processing on frame
}


} // Audio
} // Cozmo
} // Anki
