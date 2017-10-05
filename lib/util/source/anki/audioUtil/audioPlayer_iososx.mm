/**
* File: audioPlayer.mm
*
* Author: Lee Crippen
* Created: 11/29/2016
*
* Description: iOS and OSX implementation of interface.
*
* Copyright: Anki, Inc. 2016
*
*/

#include "audioPlayer.h"

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioQueue.h>

#include <algorithm>
#include <iostream>
#include <mutex>


#define NUM_BUFFERS (4) /* hangs on AudioQueueStop if >= 6 buffers! */

namespace Anki {
namespace AudioUtil {
  
struct AudioPlayerData
{
  AudioQueueRef                         _queue = nullptr;
  AudioQueueBufferRef                   _buffers[NUM_BUFFERS];
  bool                                  _playing = false;
  AudioChunkList::const_iterator        _currentChunkIter;
  AudioChunkList::const_iterator        _endChunkIter;
  uint32_t                              _nextSampleIndex = 0;
  std::mutex                            _dataMutex;
  uint32_t                              _samplesPerChunk = kSamplesPerChunk;
  
  void EnqueueAllBuffers()
  {
    for(int i = 0; i < NUM_BUFFERS; i++)
    {
      EnqueueBuffer(_queue, _buffers[i]);
    }
  }
  
  bool EnqueueBuffer(AudioQueueRef inAQ, AudioQueueBufferRef inBuffer)
  {
    std::lock_guard<std::mutex> lock{_dataMutex};
    if(_playing)
    {
      const auto samplesRemainingInChunk = _currentChunkIter->size() - _nextSampleIndex;
      uint32_t numToCopy = static_cast<uint32_t>(samplesRemainingInChunk <= _samplesPerChunk ?
                                                 samplesRemainingInChunk : _samplesPerChunk);
      
      const auto nextSampleIter = _currentChunkIter->begin() + _nextSampleIndex;
      std::copy(nextSampleIter, nextSampleIter + numToCopy, reinterpret_cast<short*>(inBuffer->mAudioData));
      _nextSampleIndex += numToCopy;
      
      inBuffer->mAudioDataByteSize = static_cast<uint32_t>(numToCopy * sizeof(AudioSample));
      AudioQueueEnqueueBuffer(_queue, inBuffer, 0, NULL);
      
      if (_nextSampleIndex == _currentChunkIter->size())
      {
        ++_currentChunkIter;
        _nextSampleIndex = 0;
      }
      
      if (_currentChunkIter == _endChunkIter)
      {
        _playing = false;
      }
      return true;
    }
    return false;
  }
};
  
static void HandleCallbackEntry(void * __nullable       inUserData,
                                AudioQueueRef           inAQ,
                                AudioQueueBufferRef     inBuffer)
  
{
  AudioPlayerData* data = static_cast<AudioPlayerData*>(inUserData);
  if (data) { data->EnqueueBuffer(inAQ, inBuffer); }
}

AudioPlayer::AudioPlayer(uint32_t samplesPerChunk)
: _impl(new AudioPlayerData{})
{
  _impl->_samplesPerChunk = samplesPerChunk;
  AudioStreamBasicDescription standardAudioFormat;
  GetStandardAudioDescriptionFormat(standardAudioFormat);
  OSStatus status = AudioQueueNewOutput(&standardAudioFormat, HandleCallbackEntry, _impl.get(), nullptr, nullptr, 0, &_impl->_queue);
  if (status)
  {
    std::cerr << "*** ERROR: failed to create audio queue ***" << std::endl;
  }
  
  for(int i = 0; i < NUM_BUFFERS; i++)
  {
    AudioQueueAllocateBuffer(_impl->_queue, kBytesPerChunk, &_impl->_buffers[i]);
  }
}
  
AudioPlayer& AudioPlayer::operator=(AudioPlayer&&) = default;
AudioPlayer::AudioPlayer(AudioPlayer&&) = default;
  
AudioPlayer::~AudioPlayer()
{
  StopPlaying();
  
  for(int i = 0; i < NUM_BUFFERS; i++)
  {
    AudioQueueFreeBuffer(_impl->_queue, _impl->_buffers[i]);
  }
  
  AudioQueueDispose(_impl->_queue, true);
}
  
bool AudioPlayer::StartPlaying(AudioChunkList::const_iterator beginIter, AudioChunkList::const_iterator endIter)
{
  {
    std::lock_guard<std::mutex> lock{_impl->_dataMutex};
    AudioQueueReset(_impl->_queue);
    
    _impl->_playing = true;
    _impl->_currentChunkIter = beginIter;
    _impl->_endChunkIter = endIter;
    _impl->_nextSampleIndex = 0;
  }
  _impl->EnqueueAllBuffers();
  
  OSStatus status = AudioQueueStart(_impl->_queue, NULL);
  if (status)
  {
    std::cerr << "*** ERROR: failed to start audio input queue ***" << std::endl;
  }
  return true;
}
  
void AudioPlayer::StopPlaying()
{
  std::lock_guard<std::mutex> lock{_impl->_dataMutex};
  _impl->_playing = false;
  AudioQueueReset(_impl->_queue);
  _impl->_currentChunkIter = _impl->_endChunkIter = AudioChunkList::const_iterator{};
}

} // end namespace AudioUtil
} // end namespace Anki
