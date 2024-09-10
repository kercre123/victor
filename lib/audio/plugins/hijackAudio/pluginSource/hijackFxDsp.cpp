/*
 * File: HijackFxDsp.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: Hijack Plug-in DSP is our hook into WWise audio process. We convert the sample rate from 32b-it floating
 *              point @ 48k to 8-bit MuLaw @ _resempleRate encoding.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "hijackFxDsp.h"
#include "audioEngine/audioDefines.h"
#include <AK/Tools/Common/AkObject.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <assert.h>


#define DEBUG_ENABLE_LOGS  0
#define DEBUG_ENABLE_FRAME_LOGS 0

#if DEBUG_ENABLE_FRAME_LOGS
uint32_t _processCount = 0;
#endif

namespace Anki {
namespace AudioEngine {
namespace PlugIns {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HijackFxDsp::HijackFxDsp( const HijackFx* parentPlugInFx ) :
  _parentPlugInFx( parentPlugInFx )
{
  assert( parentPlugInFx != nullptr );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HijackFxDsp::Setup( const AkAudioFormat& in_rFormat )
{
#if DEBUG_ENABLE_LOGS
  AUDIO_LOG("WWISE PLUGIN - HijackFxDsp.Setup SamepleRate %d\n",
            in_rFormat.uSampleRate);
#endif

  _sampleRate = in_rFormat.uSampleRate;
  
#if DEBUG_ENABLE_FRAME_LOGS
  _processCount = 0;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT HijackFxDsp::InitHijack( AK::IAkPluginMemAlloc* in_pAllocator, AkAudioFormat& in_rFormat )
{
#if DEBUG_ENABLE_LOGS
  AUDIO_LOG("WWISE PLUGIN - HijackFxDsp.InitHijack");
#endif
  
  _numProcessedChannels = in_rFormat.channelConfig.uNumChannels;
  if ( _numProcessedChannels == 0 ) {
    return AK_Fail;
  }

#if USE_RESAMPLER
  _pluginOutdata = AK_PLUGIN_ALLOC(in_pAllocator, kWwiseSamplesPerFrame * _numProcessedChannels * sizeof(AkReal32));
  _resampleBuffer.AttachContiguousDeinterleavedData(_pluginOutdata, _samplesPerFrame, 0, in_rFormat.channelConfig);
  _resampler.Init(&in_rFormat, _resampleRate);
  _resampler.SetPitch(0.0, false);
#endif
  
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HijackFxDsp::ResetHijack()
{
  // Reset resampling properties
#if USE_RESAMPLER
  _resampleBuffer.uValidFrames = 0;
  _resampler.SetOutputBufferOffset(0);
#endif

#if DEBUG_ENABLE_LOGS
  AUDIO_LOG("WWISE PLUGIN - HijackFxDsp.ResetHijack\n");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HijackFxDsp::TermHijack( AK::IAkPluginMemAlloc * in_pAllocator )
{
#if USE_RESAMPLER
  _resampleBuffer.DetachContiguousDeinterleavedData();
  AK_PLUGIN_FREE(in_pAllocator, _pluginOutdata);
  _pluginOutdata = nullptr;
#endif

#if DEBUG_ENABLE_LOGS
  AUDIO_LOG("WWISE PLUGIN - HijackFxDsp.TermHijack\n");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HijackFxDsp::Process( AkAudioBuffer * io_pBuffer )
{
  // Input format:  32-bit float @ 48,000 samples/second
  // Output format: 32-bit float @ _resampleRate samples/second
#if USE_RESAMPLER
#if DEBUG_ENABLE_FRAME_LOGS
  AUDIO_LOG("Enter - HijackFxDsp.Process\n");
#endif

  if ( nullptr != _bufferCallback ) {

#if DEBUG_ENABLE_FRAME_LOGS
    AUDIO_LOG("PRE -  execute validFrames %d - MaxFrames %d\n ", io_pBuffer->uValidFrames, io_pBuffer->MaxFrames());
#endif
    
    _resampler.SetRequestedFrames(_samplesPerFrame);
    
    // Continue to resample buffer data until it is empty
    do {
      
      // Resample
      AKRESULT result = _resampler.Execute(io_pBuffer, &_resampleBuffer);
      
      // Send resample rate's frame when samplesPerFrame size is reached
      // When AK_NoMoreData is reached send remaining sample
      if (result == AK_DataReady || result == AK_NoMoreData) {
        
        // Send audio samples to buffer
        uint32_t sampleCount = _resampleBuffer.uValidFrames;
        AudioReal32* samples = static_cast<AudioReal32*>(_resampleBuffer.GetChannel( 0 ));
        
        _bufferCallback( _parentPlugInFx, samples, sampleCount );
        
        // Reset resampler and buffer for next frame
        _resampleBuffer.uValidFrames = 0;
        _resampler.SetOutputBufferOffset(0);

      } // if (result == AK_DataReady || result == AK_NoMoreData)
      
    } while (io_pBuffer->uValidFrames > 0);
    
  } // if ( nullptr != _bufferCallback )
  
#if DEBUG_ENABLE_FRAME_LOGS
  AUDIO_LOG("Exit - HijackFxDsp.Process\n");
#endif
#endif // USE_RESAMPLER
}


} // PlugIns
} // AudioEngine
} // Anki
