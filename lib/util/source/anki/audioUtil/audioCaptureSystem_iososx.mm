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
    if(_recording && _dataCallback)
    {
      const auto* const dataStart = static_cast<AudioSample*>(inBuffer->mAudioData);
      _dataCallback(dataStart, inNumberPackets);
    }
    AudioQueueEnqueueBuffer(_queue, inBuffer, 0, NULL);
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

AudioCaptureSystem::AudioCaptureSystem()
  : _impl(new AudioCaptureSystemData{})
{
  AudioStreamBasicDescription standardAudioFormat;
  GetStandardAudioDescriptionFormat(standardAudioFormat);
  OSStatus status = AudioQueueNewInput(&standardAudioFormat, HandleCallbackEntry, _impl.get(), nullptr, nullptr, 0, &_impl->_queue);
  if (status)
  {
    std::cerr << "*** ERROR: failed to create audio queue ***" << std::endl;
  }
  
  for(int i = 0; i < NUM_BUFFERS; i++)
  {
    AudioQueueAllocateBuffer(_impl->_queue, kBytesPerChunk, &_impl->_buffers[i]);
  }
}

AudioCaptureSystem::~AudioCaptureSystem()
{
  StopRecording();
  for(int i = 0; i < NUM_BUFFERS; i++)
  {
    AudioQueueFreeBuffer(_impl->_queue, _impl->_buffers[i]);
  }
  
  AudioQueueDispose(_impl->_queue, true);
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
    
    // TODO: HACK: FIXME: This is a stub for the platform specific functionality to check microphone (and maybe other future?)
    // permissions.
    // Note this interface for requesting (or checking) microphone permission is iOS only.
#if IPHONE_TYPE
    //void(^requestBlock)(BOOL) = ^(BOOL granted) {
    //  if (granted) {
    //    NSLog(@"req granted");
    //  }
    //  else {
    //    NSLog(@"req denied");
    //  }
    //};
    
    AVAudioSessionRecordPermission permissionStatus = [[AVAudioSession sharedInstance] recordPermission];
    switch (permissionStatus) {
      case AVAudioSessionRecordPermissionUndetermined:
        [[AVAudioSession sharedInstance] requestRecordPermission: ^(BOOL granted) {
          if (granted) {
            std::cout << "microphone permission request granted" << std::endl;
          }
          else {
            std::cout << "microphone permission request denied" << std::endl;
          }
        }];
        break;
      case AVAudioSessionRecordPermissionDenied:
        std::cout << "microphone permission request already denied" << std::endl;
        break;
      case AVAudioSessionRecordPermissionGranted:
        std::cout << "microphone permission request already granted" << std::endl;
        break;
      default:
        break;
    }
#endif
    
    _impl->_recording = true;
    for(int i = 0; i < NUM_BUFFERS; i++)
    {
      AudioQueueEnqueueBuffer(_impl->_queue, _impl->_buffers[i], 0, NULL);
    }
    
    OSStatus status = AudioQueueStart(_impl->_queue, NULL);
    if (status)
    {
      std::cerr << "*** ERROR: failed to start audio input queue ***" << std::endl;
    }
  }
}

void AudioCaptureSystem::StopRecording()
{
  if (_impl->_recording)
  {
    _impl->_recording = false;
    AudioQueueStop(_impl->_queue, true);
  }
}

} // end namespace AudioUtil
} // end namespace Anki
