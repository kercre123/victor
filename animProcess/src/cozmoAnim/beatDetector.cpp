/**
* File: beatDetector.cpp
*
* Author: Matt Michini
* Created: 12/18/2017
*
* Description: Beats-per-minute and beat onset detection using aubio library
*
* Copyright: Anki, Inc. 2017
*
*/

#include "cozmoAnim/beatDetector.h"

#include "cozmoAnim/animation/animationStreamer.h"

#include "coretech/common/engine/utils/timer.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
AnimationStreamer* BeatDetector::_animStreamer = nullptr;

BeatDetector::BeatDetector()
  : _aubioInputVec(new_fvec(kAubioTempoHopSize))
  , _aubioOutputVec(new_fvec(1))
{
}

BeatDetector::~BeatDetector()
{
  if (_aubioTempoDetector != nullptr) {
    del_aubio_tempo(_aubioTempoDetector);
    _aubioTempoDetector = nullptr;
  }
  
  // Delete the input vector
  if (_aubioInputVec != nullptr) {
    del_fvec(_aubioInputVec);
    _aubioInputVec = nullptr;
  }
  
  // Delete the output vector
  if (_aubioOutputVec != nullptr) {
    del_fvec(_aubioOutputVec);
    _aubioOutputVec = nullptr;
  }
}

void BeatDetector::AddSamples(const AudioUtil::AudioChunkList& chunkList)
{
  // Lazily instantiate the aubio tempo detector object if needed
  if (_aubioTempoDetector == nullptr) {
    const char* const kAubioTempoMethod = "default";
    _aubioTempoDetector = new_aubio_tempo(kAubioTempoMethod, kAubioTempoBufSize, kAubioTempoHopSize, kAubioTempoSampleRate);
    DEV_ASSERT(_aubioTempoDetector != nullptr, "BeatDetector.AddSamples.FailedCreatingAubioTempoObject");
    
    //PRINT_NAMED_WARNING("beats", "thresh %f", aubio_tempo_get_threshold(_aubioTempoDetector));
    //DEV_ASSERT(0 == aubio_tempo_set_threshold(_aubioTempoDetector, 5.f), "failed setting threshold");
    
    _aubioInputBuffer.clear();
    
    // mark the time of the first sample
    _tempoDetectionStartedTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }
  
  // Pump data into the staging buffer
  for(const auto& audioChunk : chunkList) {
    // audioChunk is a vector of AudioSamples. Push these new samples onto the back of the aubio input buffer
    for (const auto& sample : audioChunk) {
      _aubioInputBuffer.push_back(sample);
    }
  }
  
  // Feed the aubio tempo detector correct-sized chunks (size kAubioTempoHopSize)
  while (_aubioInputBuffer.size() >= kAubioTempoHopSize) {
    for (int i=0 ; i<kAubioTempoHopSize ; i++) {
      // Copy from the front of the input buffer (and cast to aubio sample type smpl_t)
      fvec_set_sample(_aubioInputVec, static_cast<smpl_t>(_aubioInputBuffer.front()), i);
      _aubioInputBuffer.pop_front();
    }
    
    // pass this into the aubio tempo detector
    aubio_tempo_do(_aubioTempoDetector, _aubioInputVec, _aubioOutputVec);
    
    // Check the output to see if a beat was detected
    const bool isBeat = fvec_get_sample(_aubioOutputVec, 0) != 0.f;
    if (isBeat) {
      const auto tempo = aubio_tempo_get_bpm(_aubioTempoDetector);
      const auto conf = aubio_tempo_get_confidence(_aubioTempoDetector);
      PRINT_NAMED_WARNING("beat!",
                          "got a beat (tempo %.2f, confidence %.2f, last beat time (ms) %d",
                          tempo,
                          conf,
                          (uint_t) aubio_tempo_get_last_ms(_aubioTempoDetector));
      
      if (conf > 0.10f) {
        static int cnt = 0;
        if (cnt < 8) {
          _animStreamer->SetStreamingAnimation("anim_head_nod", 0);
        } else if (cnt < 16) {
          _animStreamer->SetStreamingAnimation("anim_lift_head", 0);
        } else if (cnt < 24) {
          _animStreamer->SetStreamingAnimation(cnt&1 ? "anim_lift_head_body_left" : "anim_lift_head_body_right" , 0);
        } else if (cnt < 32) {
          _animStreamer->SetStreamingAnimation(cnt&1 ? "anim_lift_head_body_left_medium" : "anim_lift_head_body_right_medium" , 0);
        } else {
          _animStreamer->SetStreamingAnimation(cnt&1 ? "anim_lift_head_body_left_large" : "anim_lift_head_body_right_large" , 0);
        }
        ++cnt;
      }
    }
  }
}
  
void BeatDetector::Stop()
{
  // Delete the beat detection object
  if (_aubioTempoDetector != nullptr) {
    del_aubio_tempo(_aubioTempoDetector);
    _aubioTempoDetector = nullptr;
  }
  
  _aubioInputBuffer.clear();
}
  
  
} // namespace Cozmo
} // namespace Anki
