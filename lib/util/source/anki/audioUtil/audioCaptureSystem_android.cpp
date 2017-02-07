/**
* File: audioCaptureSystem.cpp
*
* Author: Lee Crippen
* Created: 2/6/2017
*
* Description: Stub Android implementation of interface.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "audioCaptureSystem.h"


#include <algorithm>
#include <iostream>
#include <mutex>

namespace Anki {
namespace AudioUtil {
  
struct AudioCaptureSystemData
{
  AudioCaptureSystem::DataCallback  _dataCallback;
  bool                              _recording = false;
  mutable std::mutex                _dataMutex;
};

AudioCaptureSystem::AudioCaptureSystem()
  : _impl(new AudioCaptureSystemData{})
{
}

AudioCaptureSystem::~AudioCaptureSystem()
{
  StopRecording();
}
  
void AudioCaptureSystem::SetCallback(DataCallback newCallback)
{
  std::lock_guard<std::mutex> lock(_impl->_dataMutex);
  _impl->_dataCallback = newCallback;
}
  
void AudioCaptureSystem::StartRecording()
{
  if (!_impl->_recording)
  {
    _impl->_recording = true;
    
    // Do recording setup and start here
  }
}

void AudioCaptureSystem::StopRecording()
{
  if (_impl->_recording)
  {
    _impl->_recording = false;
    
    // Do recording teardown and stop here
  }
}

} // end namespace AudioUtil
} // end namespace Anki
