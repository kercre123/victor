/**
* File: audioCaptureSystem.h
*
* Author: Lee Crippen
* Created: 11/16/2016
*
* Description: Platform independent interface for capturing audio data (microphone input) natively.
*
* Copyright: Anki, Inc. 2016
*
*/

#ifndef __Anki_AudioUtil_AudioCaptureSystem_H_
#define __Anki_AudioUtil_AudioCaptureSystem_H_

#include "audioUtil/iAudioInputSource.h"

#include <memory>

namespace Anki {
namespace AudioUtil {
  
// Type declaration to allow hiding impl in cpp
struct AudioCaptureSystemData;

class AudioCaptureSystem : public IAudioInputSource
{
public:
  AudioCaptureSystem(uint32_t samplesPerChunk = kSamplesPerChunk, uint32_t sampleRate = kSampleRate_hz);
  virtual ~AudioCaptureSystem();
  bool IsValid() const { return _impl != nullptr; }
  
  void Init();
  
  void StartRecording();
  void StopRecording();
  
  enum class PermissionState {
    Unknown,
    Granted,
    DeniedAllowRetry,
    DeniedNoRetry,
  };
  
  PermissionState GetPermissionState(bool isRepeatRequest = false) const;
  
  using RequestCapturePermissionCallback = std::function<void()>;
  void RequestCapturePermission(RequestCapturePermissionCallback resultCallback) const;
  
private:
  std::unique_ptr<AudioCaptureSystemData>   _impl;
  uint32_t                                  _samplesPerChunk = 0;
  uint32_t                                  _sampleRate_hz = 0;
  
}; // class AudioCaptureSystem

} // end namespace AudioUtil
} // end namespace Anki


#endif //__Anki_AudioUtil_AudioCaptureSystem_H_
