/*
 * File: hijackFxDsp.h
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: Hijack Plug-in DSP is our hook into WWise audio process. We convert the sample rate from 32b-it floating
 *              point @ 48k to 8-bit MuLaw @ 24k encoding.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __AnkiAudio_PlugIns_HijackFxDsp_H__
#define __AnkiAudio_PlugIns_HijackFxDsp_H__

#ifdef HIJACK_PLUGIN
#define USE_RESAMPLER 0
#else
#define USE_RESAMPLER 1
#endif

#include "audioEngine/audioTypes.h"
#include "AkResampler.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/DSP/AkDelayLineMemoryStream.h>
#include <cstdint>
#include <functional>


// If we change the frame size need to update this define
// FIXME: There might be a way to get this at run time
#define kWwiseSamplesPerFrame 1024

namespace Anki {
namespace AudioEngine {
namespace PlugIns {


class HijackFx;

class HijackFxDsp
{
public:
  
  HijackFxDsp( const HijackFx* _parentPlugInFx );
    
  using AudioBufferCallback = std::function<void( const HijackFx* pluginInstance, const AudioReal32* samples, const uint32_t sampleCount )>;
    
  void Setup( const AkAudioFormat& in_rFormat );
  
  AKRESULT InitHijack( AK::IAkPluginMemAlloc* in_pAllocator, AkAudioFormat& in_rFormat );
  
  void ResetHijack();
  
  void TermHijack( AK::IAkPluginMemAlloc* in_pAllocator );
  
  void Process( AkAudioBuffer* io_pBuffer );
  
  // Hijack App Hooks
  void SetAudioBufferCallback( const AudioBufferCallback bufferCallback ) { _bufferCallback = bufferCallback; }
  
  //  Resampled audio DSP Settings
  void SetResampleRate( const uint32_t resampleRate ) { _resampleRate = resampleRate; }
  void SetSamplesPerFrame( const uint16_t samplesPerFrame) { _samplesPerFrame = samplesPerFrame; }

private:

  // Handle to parent pluginFx
  const HijackFx* _parentPlugInFx = nullptr;
  // Rate to resample the audio to
  uint32_t _resampleRate          = 0;
  // The number of samples frames per frame after resampling
  uint16_t _samplesPerFrame       = 0;
  // Number of channels
  AkUInt32 _numProcessedChannels  = 0;
  // Sample rate of the engine
  AkUInt32 _sampleRate            = 0;
  
#if USE_RESAMPLER
  CAkResampler _resampler;
  AkAudioBuffer _resampleBuffer;
  void* _pluginOutdata = nullptr;
#endif
  
  AudioBufferCallback _bufferCallback = nullptr;
  
  
} AK_ALIGN_DMA;

  
} // PlugIns
} // AudioEngine
} // Audio

#endif /* __HijackFxDsp_H__ */
