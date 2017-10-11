/**
* File: audioPlayer.h
*
* Author: Lee Crippen
* Created: 11/29/2016
*
* Description: Platform independent interface for playing audio data natively.
*
* Copyright: Anki, Inc. 2016
*
*/

#ifndef __Anki_AudioUtil_AudioPlayer_H_
#define __Anki_AudioUtil_AudioPlayer_H_

#include "audioDataTypes.h"

#include <memory>

namespace Anki {
namespace AudioUtil {
  
// Type declaration to allow hiding impl in cpp
struct AudioPlayerData;

class AudioPlayer
{
public:
  AudioPlayer(uint32_t samplesPerChunk = kSamplesPerChunk);
  virtual ~AudioPlayer();
  AudioPlayer& operator=(AudioPlayer&&);
  AudioPlayer(AudioPlayer&&);
  AudioPlayer& operator=(const AudioPlayer&) = delete;
  AudioPlayer(const AudioPlayer&) = delete;
  
  bool StartPlaying(AudioChunkList::const_iterator beginIter, AudioChunkList::const_iterator endIter);
  void StopPlaying();
  
private:
  std::unique_ptr<AudioPlayerData>   _impl;
  
}; // class AudioPlayer

} // end namespace AudioUtil
} // end namespace Anki


#endif //__Anki_AudioUtil_AudioPlayer_H_
