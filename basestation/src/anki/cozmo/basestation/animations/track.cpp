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

// Specialization for ProceduralFace track because it needs look-back for interpolation
template<>
RobotInterface::EngineToRobot* Animations::Track<ProceduralFaceKeyFrame>::GetCurrentStreamingMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms)
{
  RobotInterface::EngineToRobot* msg = nullptr;

  if(HasFramesLeft()) {
    ProceduralFaceKeyFrame& currentKeyFrame = GetCurrentKeyFrame();
    if(currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms))
    {
      if(currentKeyFrame.IsLive()) {
        // The AnimationStreamer will take care of interpolation for live
        // live streaming
        // TODO: Maybe we could also do it here somehow?
        msg = currentKeyFrame.GetStreamMessage();

      } else {
        auto nextIter = _frameIter;
        ++nextIter;
        if(nextIter != _frames.end()) {
          // If we have another frame coming, use it to interpolate.
          // This will only be "done" once
          msg = currentKeyFrame.GetInterpolatedStreamMessage(*nextIter);
        } else {
          // Otherwise, we'll just send the last frame
          msg = currentKeyFrame.GetStreamMessage();
        }
      }

      if(currentKeyFrame.IsDone()) {
        MoveToNextKeyFrame();
      }
    }
  }

  return msg;
}


} // end namespace Animations
} // end namespace Cozmo
} // end namespace Anki



