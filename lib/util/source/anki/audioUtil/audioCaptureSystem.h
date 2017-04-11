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

#include "audioDataTypes.h"

#include <functional>
#include <memory>

namespace Anki {
namespace AudioUtil {
  
// Type declaration to allow hiding impl in cpp
struct AudioCaptureSystemData;

class AudioCaptureSystem
{
public:
  AudioCaptureSystem();
  ~AudioCaptureSystem();
  bool IsValid() const { return _impl != nullptr; }
  
  using DataCallback = std::function<void(const AudioSample* ,uint32_t)>;
  void SetCallback(DataCallback newCallback = DataCallback{});
  
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
  
}; // class AudioCaptureSystem

} // end namespace AudioUtil
} // end namespace Anki


#endif //__Anki_AudioUtil_AudioCaptureSystem_H_
