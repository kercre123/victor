
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
#include "anki/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/robotInterface/messageToActiveObject.h"


namespace Anki
{
  namespace Cozmo
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
      

// #pragma mark --- Audio ---
      /////////////////////////////////////////////////////////////////////
      // AUDIO
      // TODO: To be removed when animation streaming migrates to engine

      void AudioFill(void);

      // @return true if the audio clock says it is time for the next frame
      bool AudioReady();

      // Play one frame of audio or silence
      // @param frame - a pointer to an audio frame or NULL to play one frame of silence
      void AudioPlayFrame(AnimKeyFrame::AudioSample *msg);
      void AudioPlaySilence();

      
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_SIM_HARDWAREINTERFACE_H
