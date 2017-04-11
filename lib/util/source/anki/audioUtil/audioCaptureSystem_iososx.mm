/**
* File: audioCaptureSystem.mm
*
* Author: Lee Crippen
* Created: 11/16/2016
*
* Description: iOS and OSX implementation of interface.
*
* Copyright: Anki, Inc. 2016
*
*/

#include "audioCaptureSystem.h"
#include "util/logging/logging.h"

#import <AudioToolbox/AudioToolbox.h>


#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#define IPHONE_TYPE 1
#else
#define IPHONE_TYPE 0
#endif

#if IPHONE_TYPE
#import <AVFoundation/AVFoundation.h>
#endif

#include <algorithm>
#include <iostream>
#include <mutex>


#define NUM_BUFFERS (4) /* hangs on AudioQueueStop if >= 6 buffers! */

namespace Anki {
namespace AudioUtil {
  
struct AudioCaptureSystemData
{
  AudioCaptureSystem::DataCallback  _dataCallback;
  AudioQueueRef                     _queue = nullptr;
  AudioQueueBufferRef               _buffers[NUM_BUFFERS];
  bool                              _recording = false;
  mutable std::mutex                _dataMutex;
  
  void HandleCallback(AudioQueueRef                                   inAQ,
                      AudioQueueBufferRef                             inBuffer,
                      const AudioTimeStamp *                          inStartTime,
                      UInt32                                          inNumberPackets,
                      const AudioStreamPacketDescription * __nullable inPacketDescs)
  {
    std::lock_guard<std::mutex> lock(_dataMutex);
    if(_recording)
    {
      if (_dataCallback)
      {
        const auto* const dataStart = static_cast<AudioSample*>(inBuffer->mAudioData);
        _dataCallback(dataStart, inNumberPackets);
      }
      
      AudioQueueEnqueueBuffer(_queue, inBuffer, 0, NULL);
    }
  }
  
  ~AudioCaptureSystemData()
  {
    if (_queue != nullptr)
    {
      for(int i = 0; i < NUM_BUFFERS; i++)
      {
        if (_buffers[i] != nullptr)
        {
          AudioQueueFreeBuffer(_queue, _buffers[i]);
        }
      }
      
      AudioQueueDispose(_queue, true);
    }
  }
};
  
static void HandleCallbackEntry(void * __nullable               inUserData,
                                AudioQueueRef                   inAQ,
                                AudioQueueBufferRef             inBuffer,
                                const AudioTimeStamp *          inStartTime,
                                UInt32                          inNumberPackets,
                                const AudioStreamPacketDescription * __nullable inPacketDescs)
{
  AudioCaptureSystemData* data = static_cast<AudioCaptureSystemData*>(inUserData);
  if (data) { data->HandleCallback(inAQ, inBuffer, inStartTime, inNumberPackets, inPacketDescs); }
}

AudioCaptureSystem::AudioCaptureSystem() = default;

// Note this should be done AFTER permission has been granted
void AudioCaptureSystem::Init()
{
  if (!_impl && GetPermissionState() == PermissionState::Granted)
  {
    _impl.reset(new AudioCaptureSystemData{});
    
    AudioStreamBasicDescription standardAudioFormat;
    GetStandardAudioDescriptionFormat(standardAudioFormat);
    OSStatus status = AudioQueueNewInput(&standardAudioFormat, HandleCallbackEntry, _impl.get(), nullptr, nullptr, 0, &_impl->_queue);
    if (kAudioServicesNoError != status)
    {
      PRINT_NAMED_ERROR("AudioCaptureSystem.Constructor.AudioQueueNewInput.Error","OSStatus errorcode: %d", (int)status);
      _impl.reset();
      return;
    }
    
    for(int i = 0; i < NUM_BUFFERS; i++)
    {
      status = AudioQueueAllocateBuffer(_impl->_queue, kBytesPerChunk, &_impl->_buffers[i]);
      if (kAudioServicesNoError != status)
      {
        PRINT_NAMED_ERROR("AudioCaptureSystem.Constructor.AudioQueueAllocateBuffer.Error","OSStatus errorcode: %d", (int)status);
        _impl.reset();
        return;
      }
    }
  }
}

AudioCaptureSystem::~AudioCaptureSystem()
{
  StopRecording();
}
  
void AudioCaptureSystem::SetCallback(DataCallback newCallback)
{
  if (_impl)
  {
    std::lock_guard<std::mutex> lock(_impl->_dataMutex);
    _impl->_dataCallback = newCallback;
  }
}
 
#if IPHONE_TYPE
static AudioCaptureSystem::PermissionState ConvertPermissionType(AVAudioSessionRecordPermission avPermissionStatus)
{
  switch (avPermissionStatus)
  {
    case AVAudioSessionRecordPermissionUndetermined:
    {
      return AudioCaptureSystem::PermissionState::Unknown;
    }
    case AVAudioSessionRecordPermissionDenied:
    {
      return AudioCaptureSystem::PermissionState::DeniedNoRetry;
    }
    case AVAudioSessionRecordPermissionGranted:
    {
      return AudioCaptureSystem::PermissionState::Granted;
    }
    default:
    {
      return AudioCaptureSystem::PermissionState::Unknown;
    }
  }
}
#endif

AudioCaptureSystem::PermissionState AudioCaptureSystem::GetPermissionState(bool isRepeatRequest) const
{
  // MacOS always grants permission to use the microphone, so use that as default
  PermissionState permissionState = PermissionState::Granted;

  #if IPHONE_TYPE
    permissionState = ConvertPermissionType([[AVAudioSession sharedInstance] recordPermission]);
  #endif
  
  return permissionState;
}

void AudioCaptureSystem::RequestCapturePermission(RequestCapturePermissionCallback resultCallback) const
{
  if (IPHONE_TYPE)
  {
  #if IPHONE_TYPE
    [[AVAudioSession sharedInstance] requestRecordPermission: ^(BOOL granted) {
      resultCallback();
    }];
  #endif
  }
  else
  {
    resultCallback();
  }
}

void AudioCaptureSystem::StartRecording()
{
  if (_impl && !_impl->_recording && GetPermissionState() == PermissionState::Granted)
  {
    _impl->_recording = true;
    for(int i = 0; i < NUM_BUFFERS; i++)
    {
      AudioQueueEnqueueBuffer(_impl->_queue, _impl->_buffers[i], 0, NULL);
    }
    
    // IOS needs to have the AVAudioSession set up properly in order to begin queueing audio
#if IPHONE_TYPE
    AVAudioSession * session = [AVAudioSession sharedInstance];
    
    if (!session)
    {
      PRINT_NAMED_ERROR("AudioCaptureSystem.StartRecording.AVAudioSession.sharedInstance.Invalid", "");
    }
    else
    {
      NSError* nsError;
      [session setCategory:AVAudioSessionCategoryPlayAndRecord error:&nsError];
      
      if (nsError)
      {
        PRINT_NAMED_ERROR("AudioCaptureSystem.StartRecording.AVAudioSession.setCategory.Error", "Code %ld %s",
                            (long)[nsError code], [[nsError localizedDescription] UTF8String]);
      }
      
      [session setActive:YES error:&nsError];
      if (nsError)
      {
        PRINT_NAMED_ERROR("AudioCaptureSystem.StartRecording.AVAudioSession.setActive.Error", "Code %ld %s",
                          (long)[nsError code], [[nsError localizedDescription] UTF8String]);
      }
    }
#endif
    
    OSStatus status = AudioQueueStart(_impl->_queue, NULL);
    if (kAudioServicesNoError != status)
    {
      PRINT_NAMED_ERROR("AudioCaptureSystem.StartRecording.AudioQueueStart.Error","Is permission properly granted? OSStatus errorcode: %d", (int)status);
    }
  }
}

void AudioCaptureSystem::StopRecording()
{
  if (_impl && _impl->_recording)
  {
    _impl->_recording = false;
    AudioQueueStop(_impl->_queue, true);
  }
}

} // end namespace AudioUtil
} // end namespace Anki
