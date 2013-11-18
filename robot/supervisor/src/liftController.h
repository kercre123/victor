/**
 * File: liftController.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/28/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description:
 *
 *   Controller for adjusting the lifter height.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef COZMO_LIFT_CONTROLLER_H_
#define COZMO_LIFT_CONTROLLER_H_

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {
    namespace LiftController {
      
      // TODO: Add if/when needed?
      // ReturnCode Init();
      
      void SetAngularVelocity(const f32 rad_per_sec);
      void SetDesiredHeight(const f32 height_mm);
      bool IsInPosition();
      
      ReturnCode Update();
      
    } // namespace LiftController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_LIFT_CONTROLLER_H_