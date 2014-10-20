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
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef COZMO_ANIMATION_CONTROLLER_H_
#define COZMO_ANIMATION_CONTROLLER_H_

#include "anki/common/types.h"
#include "anki/cozmo/shared/cozmoTypes.h"

#include "keyFrame.h"
#include "animation.h"

namespace Anki {
  namespace Cozmo {
    namespace AnimationController {
      
      Result Init();
      
      void Update();
      
      // Plays animation numLoops times.
      // If numLoops == 0, then repeats until Stop() is called.
      void Play(const AnimationID_t anim, const u32 numLoops);
      
      void Stop();
      
      bool IsPlaying();
      
      // For updating "canned" animations (e.g. using definitions sent over
      // from the Basestation):
      Result ClearCannedAnimation(const AnimationID_t whichAnimation);
      
      // (Adds frame to end of specified animation/subsystem)
      Result AddKeyFrameToCannedAnimation(const KeyFrame&             keyframe,
                                          const AnimationID_t         whichAnimation,
                                          const Animation::SubSystems whichSubSystem);

    } // namespace AnimationController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_ANIMATION_CONTROLLER_H_
