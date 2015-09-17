/**
* File: track
*
* Author: damjan stulic
* Created: 9/16/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/


#include "anki/cozmo/basestation/animations/track.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace Animations {

template<typename FRAME_TYPE>
void Track<FRAME_TYPE>::Init()
{
  _frameIter = _frames.begin();
}

template<typename FRAME_TYPE>
void Track<FRAME_TYPE>::MoveToNextKeyFrame()
{
  if(_frameIter->IsLive()) {
    // Live frames get removed from the track once played
    _frameIter = _frames.erase(_frameIter);
  } else {
    // For canned frames, we just move to the next one in the track
    ++_frameIter;
  }
}

// returns false if frame buffer is full
template<typename FRAME_TYPE>
bool Track<FRAME_TYPE>::CheckFrameCount(const char* className)
{
  if(_frames.size() > MAX_FRAMES_PER_TRACK) {
    PRINT_NAMED_ERROR("Animation.Track.AddKeyFrame.TooManyFrames",
      "There are already %lu frames in %s track. Refusing to add more.",
      _frames.size(), className);
    return false;
  }
  return true;
}


} // end namespace Animations
} // end namespace Cozmo
} // end namespace Anki



