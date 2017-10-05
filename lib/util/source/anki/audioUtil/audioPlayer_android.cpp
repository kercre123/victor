/**
* File: audioPlayer.cpp
*
* Author: Lee Crippen
* Created: 2/6/2017
*
* Description: Stub Android implementation of interface.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "audioPlayer.h"

#include <algorithm>
#include <iostream>
#include <mutex>

namespace Anki {
namespace AudioUtil {
  
struct AudioPlayerData
{
  bool                                  _playing = false;
  AudioChunkList::const_iterator        _currentChunkIter;
  AudioChunkList::const_iterator        _endChunkIter;
  uint32_t                              _nextSampleIndex = 0;
  std::mutex                            _dataMutex;
  uint32_t                              _samplesPerChunk = kSamplesPerChunk;
};
  
AudioPlayer::AudioPlayer(uint32_t samplesPerChunk)
: _impl(new AudioPlayerData{})
{
  _impl->_samplesPerChunk = samplesPerChunk;
}
  
AudioPlayer& AudioPlayer::operator=(AudioPlayer&&) = default;
AudioPlayer::AudioPlayer(AudioPlayer&&) = default;
  
AudioPlayer::~AudioPlayer()
{
  StopPlaying();
}
  
bool AudioPlayer::StartPlaying(AudioChunkList::const_iterator beginIter, AudioChunkList::const_iterator endIter)
{
  {
    std::lock_guard<std::mutex> lock{_impl->_dataMutex};
    
    _impl->_playing = true;
    _impl->_currentChunkIter = beginIter;
    _impl->_endChunkIter = endIter;
    _impl->_nextSampleIndex = 0;
    
    // Do native setup and start here
  }
  return true;
}
  
void AudioPlayer::StopPlaying()
{
  std::lock_guard<std::mutex> lock{_impl->_dataMutex};
  _impl->_playing = false;
  _impl->_currentChunkIter = _impl->_endChunkIter = AudioChunkList::const_iterator{};
  
  // Do native cleanup and stop here
}

} // end namespace AudioUtil
} // end namespace Anki
