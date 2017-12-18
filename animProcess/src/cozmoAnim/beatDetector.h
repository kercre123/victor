/**
* File: beatDetector.h
*
* Author: Matt Michini
* Created: 12/18/2017
*
* Description: Beats-per-minute and beat onset detection using aubio library
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __AnimProcess_CozmoAnim_BeatDetector_H_
#define __AnimProcess_CozmoAnim_BeatDetector_H_

#include "cozmoAnim/micDataTypes.h"

#include "aubio/aubio.h"

#include <deque>

namespace Anki {
namespace Cozmo {

class BeatDetector
{
public:
  BeatDetector();
  ~BeatDetector();
  BeatDetector(const BeatDetector& other) = delete;
  BeatDetector& operator=(const BeatDetector& other) = delete;
  
  void AddSamples(const AudioUtil::AudioChunkList&);
  
  void Stop();

private:

  static const uint_t kAubioTempoBufSize = 512;
  static const uint_t kAubioTempoHopSize = 256;
  static const uint_t kAubioTempoSampleRate = AudioUtil::kSampleRate_hz;
  
  // Aubio beat detection stuff:
  aubio_tempo_t* _aubioTempoDetector = nullptr;
  
  // Aubio beat detector input/output vectors:
  fvec_t* _aubioInputVec = nullptr;
  fvec_t* _aubioOutputVec = nullptr;
  
  // Stages audio data to be piped into the aubio detector at the correct chunk size
  std::deque<AudioUtil::AudioSample> _aubioInputBuffer;
};

} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_BeatDetector_H_
