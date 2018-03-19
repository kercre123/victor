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

#import <TargetConditionals.h>

//
// Audio capture is OFF unless enabled by configuration
//
#if !defined(AUDIOCAPTURE_FUNCTIONALITY)
#define AUDIOCAPTURE_FUNCTIONALITY 0
#endif

#if AUDIOCAPTURE_FUNCTIONALITY && (TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
#define IPHONE_TYPE 1
#else
#define IPHONE_TYPE 0
#endif

#if AUDIOCAPTURE_FUNCTIONALITY
#import <AudioToolbox/AudioToolbox.h>
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
#if AUDIOCAPTURE_FUNCTIONALITY
  AudioCaptureSystem*               _captureSystem = nullptr;
  AudioQueueRef                     _queue = nullptr;
  AudioQueueBufferRef               _buffers[NUM_BUFFERS];
  bool                              _recording = false;
  
  void HandleCallback(AudioQueueRef                                   inAQ,
                      AudioQueueBufferRef                             inBuffer,
                      const AudioTimeStamp *                          inStartTime,
                      UInt32                                          inNumberPackets,
                      const AudioStreamPacketDescription * __nullable inPacketDescs)
  {
    if(_recording)
    {
      auto dataCallback = _captureSystem->GetDataCallback();
      if (dataCallback)
      {
        const auto* const dataStart = static_cast<AudioSample*>(inBuffer->mAudioData);
        dataCallback(dataStart, inNumberPackets);
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
    _captureSystem = nullptr;
  }
#endif
};
  
#if AUDIOCAPTURE_FUNCTIONALITY
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
#endif

AudioCaptureSystem::AudioCaptureSystem(uint32_t samplesPerChunk, uint32_t sampleRate)
: _samplesPerChunk(samplesPerChunk)
, _sampleRate_hz(sampleRate)
{
  // To prevent warnings when functionality is not defined in
  (void) _samplesPerChunk;
  (void) _sampleRate_hz;
}

// Note this should be done AFTER permission has been granted
void AudioCaptureSystem::Init()
{
#if AUDIOCAPTURE_FUNCTIONALITY
  if (!_impl && GetPermissionState() == PermissionState::Granted)
  {
    _impl.reset(new AudioCaptureSystemData{});
    _impl->_captureSystem = this;
    
    AudioStreamBasicDescription standardAudioFormat;
    GetStandardAudioDescriptionFormat(standardAudioFormat);
    standardAudioFormat.mSampleRate = _sampleRate_hz;
    OSStatus status = AudioQueueNewInput(&standardAudioFormat, HandleCallbackEntry, _impl.get(), nullptr, nullptr, 0, &_impl->_queue);
    if (kAudioServicesNoError != status)
    {
      PRINT_NAMED_WARNING("AudioCaptureSystem.Constructor.AudioQueueNewInput.Warn","OSStatus errorcode: %d", (int)status);
      _impl.reset();
      return;
    }
    
    for(int i = 0; i < NUM_BUFFERS; i++)
    {
      status = AudioQueueAllocateBuffer(_impl->_queue, _samplesPerChunk * sizeof(AudioSample), &_impl->_buffers[i]);
      if (kAudioServicesNoError != status)
      {
        PRINT_NAMED_WARNING("AudioCaptureSystem.Constructor.AudioQueueAllocateBuffer.Warn","OSStatus errorcode: %d", (int)status);
        _impl.reset();
        return;
      }
    }
  }
#endif
}

AudioCaptureSystem::~AudioCaptureSystem()
{
  StopRecording();
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
  
#if IPHONE_TYPE
static void DoIPhoneSpecificConfig()
{
  AVAudioSession * session = [AVAudioSession sharedInstance];
  if (!session)
  {
    PRINT_NAMED_ERROR("AudioCaptureSystem.DoIPhoneSpecificConfig.AVAudioSession.sharedInstance.Invalid", "");
    return;
  }

  auto currentOptions = [session categoryOptions];
  // This option needs to be set explicitly for the PlayAndRecord category we're switching to below.
  // If that doesn't happen, audio reverts to default playback through phone speaker only, instead of
  // speakerphone mode.
  currentOptions |= AVAudioSessionCategoryOptionDefaultToSpeaker;
  NSError* nsError = nil;
  [session setCategory:AVAudioSessionCategoryPlayAndRecord
           withOptions:currentOptions
                 error:&nsError];
  if (nsError)
  {
    PRINT_NAMED_ERROR("AudioCaptureSystem.DoIPhoneSpecificConfig.AVAudioSession.setCategory.Error", "Code %ld %s",
                        (long)[nsError code], [[nsError localizedDescription] UTF8String]);
  }

  // The System measurement mode could? help with voice recognition (it's the mode used by the speech
  // recognition software THF by Sensory), but unfortunately enabling it results in reducing overall device
  // output volume by ~60% (very rough guess) which isn't well documented in Apple docs. So far I haven't found
  // any way to disable that side effect either, so leaving this in but disabled for historical purposes.
  // Set the "system measurement" mode to disable signal processing from input
//  [session setMode:AVAudioSessionModeMeasurement error:&nsError];
//  if (nsError)
//  {
//    PRINT_NAMED_ERROR("AudioCaptureSystem.StartRecording.AVAudioSession.setMode.Error", "Code %ld %s",
//                      (long)[nsError code], [[nsError localizedDescription] UTF8String]);
//  }

  [session setActive:YES error:&nsError];
  if (nsError)
  {
    PRINT_NAMED_ERROR("AudioCaptureSystem.DoIPhoneSpecificConfig.AVAudioSession.setActive.Error", "Code %ld %s",
                      (long)[nsError code], [[nsError localizedDescription] UTF8String]);
  }
  
  
  // Now that the session has been enabled, look for a front microphone. If it exists we'll prioritize it.
  // Note this has been adapted from the example at https://developer.apple.com/library/content/qa/qa1799/_index.html
  // Also note that the final part of the example was removed, which set the built-in mic to be the preferred
  // input port. It was removed because of the earlier check below which returns early if there's more than one
  // mic port found.
  
  // Get the set of available inputs. If there are no audio accessories attached, there will be
  // only one available input -- the built in microphone.
  NSArray* inputs = [session availableInputs];
  if ([inputs count] > 1)
  {
    // If there's more than one input, we'll assume the user wants to use the other input instead
    // of the built-in mic, so we'll return rather than configure (and prioritize) that mic
    return;
  }
  
  // Locate the Port corresponding to the built-in microphone.
  AVAudioSessionPortDescription* builtInMicPort = nil;
  for (AVAudioSessionPortDescription* port in inputs)
  {
    if ([port.portType isEqualToString:AVAudioSessionPortBuiltInMic])
    {
      builtInMicPort = port;
      break;
    }
  }
  
  // Print out a description of the data sources for the built-in microphone
//  NSLog(@"There are %u data sources for port :\"%@\"", (unsigned)[builtInMicPort.dataSources count], builtInMicPort);
//  NSLog(@"%@", builtInMicPort.dataSources);
  
  // loop over the built-in mic's data sources and attempt to locate the front microphone
  AVAudioSessionDataSourceDescription* frontDataSource = nil;
  for (AVAudioSessionDataSourceDescription* source in builtInMicPort.dataSources)
  {
    if ([source.orientation isEqual:AVAudioSessionOrientationFront])
    {
      frontDataSource = source;
      break;
    }
  } // end data source iteration
  
  if (frontDataSource)
  {
//    NSLog(@"Currently selected source is \"%@\" for port \"%@\"", builtInMicPort.selectedDataSource.dataSourceName, builtInMicPort.portName);
//    NSLog(@"Attempting to select source \"%@\" on port \"%@\"", frontDataSource, builtInMicPort.portName);
    
    // Set a preference for the front data source.
    if (![builtInMicPort setPreferredDataSource:frontDataSource error:&nsError])
    {
      PRINT_NAMED_ERROR("AudioCaptureSystem.DoIPhoneSpecificConfig.AVAudioSessionPortDescription.setPreferredDataSource.Error", "Code %ld %s",
                        (long)[nsError code], [[nsError localizedDescription] UTF8String]);
    }
  }
}
#endif

void AudioCaptureSystem::StartRecording()
{
#if AUDIOCAPTURE_FUNCTIONALITY
  if (_impl && !_impl->_recording && GetPermissionState() == PermissionState::Granted)
  {
    _impl->_recording = true;
    for(int i = 0; i < NUM_BUFFERS; i++)
    {
      AudioQueueEnqueueBuffer(_impl->_queue, _impl->_buffers[i], 0, NULL);
    }
    
#if IPHONE_TYPE
    {
      DoIPhoneSpecificConfig();
    }
#endif
    
    OSStatus status = AudioQueueStart(_impl->_queue, NULL);
    if (kAudioServicesNoError != status)
    {
      PRINT_NAMED_WARNING("AudioCaptureSystem.StartRecording.AudioQueueStart.Warn","Is permission properly granted? OSStatus errorcode: %d", (int)status);
    }
  }
#endif
}

void AudioCaptureSystem::StopRecording()
{
#if AUDIOCAPTURE_FUNCTIONALITY
  if (_impl && _impl->_recording)
  {
    _impl->_recording = false;
    AudioQueueStop(_impl->_queue, true);
  }
#endif
}

} // end namespace AudioUtil
} // end namespace Anki
