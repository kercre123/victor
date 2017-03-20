/*
 * File: audioMixingConsole.cpp
 *
 * Author: Jordan Rivas
 * Created: 05/17/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#include "anki/cozmo/basestation/audio/mixing/audioMixingConsole.h"
#include "anki/cozmo/basestation/audio/mixing/audioMixerInputSource.h"
#include "anki/cozmo/basestation/audio/mixing/audioMixerOutputSource.h"

#include "util/logging/logging.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMixingConsole::AudioMixingConsole( size_t frameSize )
: _frameSize( frameSize )
{
  PRINT_NAMED_INFO("AudioMixingConsole.AudioMixingConsole", "Create Mixing Console FrameSize: %lu",
                   (unsigned long)_frameSize );
  
  _mixingBuffer = new double[frameSize];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMixingConsole::~AudioMixingConsole()
{
  for (auto& anInput : _inputSources) {
    Util::SafeDelete(anInput);
  }
  // FIXME: WTF!! why isn't this able to delete?
//  for (auto& anOutput: _outputSources) {
//    Util::SafeDelete(anOutput);
//  }
  
//  Util::SafeDeleteArray(_sourceMixingBuffer);
  Util::SafeDeleteArray(_mixingBuffer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: Add engine time ?
AudioMixingConsole::BufferSate AudioMixingConsole::Update( )
{
  ++_audioClock;
  
  // Check if any input sources need to wait
  _state = TickInputSources();
  
  return _state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMixingConsole::ProcessFrame()
{
  // Update input sources states
  bool hasFrame = MixInputSources();
  
  // Update output sources
  SendOutputSources(hasFrame);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMixingConsole::AddInputSource( AudioMixerInputSource* inputSource )
{
  _inputSources.emplace_back( inputSource );
  
  // FIXME: This could be better
  // Temp pointer storage
//  Util::SafeDeleteArray( _sourceMixingBuffer );
//  _sourceMixingBuffer = new AudioFrameData*[_inputSources.size()];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMixingConsole::AddOutputSource( AudioMixerOutputSource* outputSource )
{
  _outputSources.emplace_back( outputSource );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMixingConsole::BufferSate AudioMixingConsole::TickInputSources()
{
  // Check if we need to wait for input sources to buffer
  for ( auto& aSource : _inputSources ) {
    if ( aSource->GetInputSourceState() == AudioMixerInputSource::BufferState::Wait ) {
      return BufferSate::Loading;
    }
  }
  return BufferSate::Ready;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// We are depending on the source state being either empty or ready
bool AudioMixingConsole::MixInputSources()
{
  ClearMixingBuffer();
  bool hasFrames = false;
  for ( auto& aSource : _inputSources ) {
    // Sum all input sources audio sample
    if ( aSource->GetInputSourceState() == AudioMixerInputSource::BufferState::Ready ) {
     // Sum source with mixing buffer
      
      // TODO: Get audio frame from source
      
      // Note: Source is the original audio data, do NOT modify!
      const AudioEngine::AudioFrameData* sourceFrame = aSource->PopFrame();
      if (sourceFrame != nullptr) {
        hasFrames = true;
        
        
        
        // FIXME: DISPLAY THE FRAME BEFORE MIXING
//        PRINT_NAMED_WARNING("AudioMixingConsole.MixInputSources", " %s", sourceFrame->Description().c_str());
        
        
        
        // Check if channel frame has audio samples, channel is not muted and volume is > 0.0
        const float volume = aSource->GetVolume();
        if ( sourceFrame!= nullptr && !aSource->GetMute() && Util::IsFltGT(volume, 0.0f) ) {
          if (Util::IsFltNear(volume, 1.0f)) {
            // Don't scale just mix
            for ( size_t idx = 0; idx < sourceFrame->sampleCount; ++idx ) {
              _mixingBuffer[idx] += sourceFrame->samples[idx];
            }
          }
          else {
            // Scale for volume & mix
            for ( size_t idx = 0; idx < sourceFrame->sampleCount; ++idx ) {
              _mixingBuffer[idx] += sourceFrame->samples[idx] * volume;
            }
          }
        }
      }
    }
  }
  return hasFrames;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMixingConsole::SendOutputSources(bool hasFrame)
{
  for ( auto& aSource : _outputSources ) {
    aSource->ProcessTick( hasFrame ? _mixingBuffer : nullptr );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioMixingConsole::ClearMixingBuffer()
{
  memset( _mixingBuffer, 0, _frameSize * sizeof( MixingConsoleSample ) );
}

  
} // Audio
} // Cozmo
} // Anki
