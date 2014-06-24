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

namespace Anki {
  namespace Cozmo {
    namespace AnimationController {
      
      void Update();
      
      void PlayAnimation(AnimationID_t anim);
      
    } // namespace AnimationController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_ANIMATION_CONTROLLER_H_
