
/**
 * File: sim_hal.h
 *
 * Author: Kevin Yoon
 * Created: 03/10/2017
 *
 * Description:
 *
 *   Cozmo 2.0 simulator-only HAL interface
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef ANKI_COZMO_ROBOT_SIM_HARDWAREINTERFACE_H
#define ANKI_COZMO_ROBOT_SIM_HARDWAREINTERFACE_H
#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"


namespace Anki
{
  namespace Vector
  {
    namespace HAL
    {
      //Result Init(void);
      void Destroy(void);

      // Misc simulator-only stuff
      bool IsInitialized();
      void UpdateDisplay();

      // Take a step (needed for webots only)
      //Result Step(void);

      // Ground truth (no-op if not in simulation)
      void GetGroundTruthPose(f32 &x, f32 &y, f32& rad);

// #pragma mark --- Gripper ---
      /////////////////////////////////////////////////////////////////////
      // GRIPPER
      //
      
      bool IsGripperEngaged();
      
      // Engage/Disengage gripper
      // NOTE: Does not guarantee physical connection.
      void EngageGripper();
      void DisengageGripper();

      
    } // namespace HAL
  } // namespace Vector
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_SIM_HARDWAREINTERFACE_H
