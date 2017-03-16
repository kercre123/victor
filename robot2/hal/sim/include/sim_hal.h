
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
      Result Init(void);
      void Destroy(void);

      // Misc simulator-only stuff
      bool IsInitialized();
      void UpdateDisplay();

      // Take a step (needed for webots only)
      Result Step(void);

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

// #pragma mark --- Face ---
      /////////////////////////////////////////////////////////////////////
      // Face
      // TODO: To be removed when animation streaming migrates to engine

      // Update the face to the next frame of an animation
      // @param frame - a pointer to a variable length frame of face animation data
      // Frame is in 8-bit RLE format:
      //  0 terminates the image
      //  1-63 draw N full lines (N*128 pixels) of black or blue
      //  64-255 draw 0-191 pixels (N-64) of black or blue, then invert the color for the next run
      // The decoder starts out drawing black, and inverts the color on every byte >= 64
      void FaceAnimate(u8* frame, const u16 length);

      // Clear the currently animated face
      void FaceClear();

      // Print a message to the face - this will permanently replace the face with your message
      extern "C" void FacePrintf(const char *format, ...);

      // Restore normal operation of the face from a FacePrintf
      extern "C" void FaceUnPrintf(void);

      
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_SIM_HARDWAREINTERFACE_H
