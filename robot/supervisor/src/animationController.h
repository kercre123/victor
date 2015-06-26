/**
 * File: animationController.h
 *
 * Author: Kevin Yoon
 * Created: 6/23/2014
 *
 * Description:
 *
 *   Controller for playing animations that comprise coordinated motor, light, and sound actions.
 *
 * Update: Andrew Stein, 6/22/2015
 *   Updating to support streaming animations from Basestation instead of a set of static, 
 *   canned animations stored on the Robot.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef COZMO_ANIMATION_CONTROLLER_H_
#define COZMO_ANIMATION_CONTROLLER_H_

#include "anki/common/types.h"
#include "anki/cozmo/shared/cozmoTypes.h"

#include "messages.h"

namespace Anki {
  namespace Cozmo {
    namespace AnimationController {
      
      // Amount of "pre-roll" that needs to be buffered before a streamed animation
      // will start playing, in number of keyframes (noting that each keyframe is 33ms)
      static const s32 PREROLL_BUFFER_LENGTH = 500/33; // 1/2 a second (500ms / (33ms per frame))
      
      // Streaming KeyFrame buffer size, in number of keyframes
      static const s32 KEYFRAME_BUFFER_LENGTH = 128;
      
      // If buffer gets within this number of keyframes of the buffer length,
      // then it is considered "full" for the purposes of IsBufferFull() below.
      static const s32 KEYFRAME_BUFFER_PADDING = 32;
      
      Result Init();

      // Buffer up a new KeyFrame for playing, using a KeyFrame message
      Result BufferKeyFrame(const Messages::AnimKeyFrame_HeadAngle&      msg);
      Result BufferKeyFrame(const Messages::AnimKeyFrame_LiftHeight&     msg);
      Result BufferKeyFrame(const Messages::AnimKeyFrame_AudioSample&    msg);
      Result BufferKeyFrame(const Messages::AnimKeyFrame_AudioSilence&   msg);
      Result BufferKeyFrame(const Messages::AnimKeyFrame_FaceImage&      msg);
      Result BufferKeyFrame(const Messages::AnimKeyFrame_FacePosition&   msg);
      Result BufferKeyFrame(const Messages::AnimKeyFrame_BackpackLights& msg);
      Result BufferKeyFrame(const Messages::AnimKeyFrame_BodyMotion&     msg);

      // Plays any buffered keyframes available, if enough of a pre-roll is
      // buffered up or we've received all the keyframes for the animation
      // that's currently playing.
      void Update();
      
      // Clears any remaining buffered keyframes and thus immediately stops
      // animation from playing
      void Clear();
      
      // Returns true if there are buffered keyframes being played
      bool IsPlaying();
      
      // Returns true if there is no more room left in the buffer for new
      // frames to be streamed. (With some padding for what may already be
      // on the way)
      bool IsBufferFull();
      
      // Get approximate number of frames available in the streaming buffer,
      // subject to some padding to leave space for frames that might be on
      // their way in the comms channel.
      s32  GetNumFramesFree();
      
      
    } // namespace AnimationController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_ANIMATION_CONTROLLER_H_
