/**
 * File: face.h
 *
 * Author: Daniel Casner
 * Created: 10/26/2015
 *
 * Description:
 *
 *   Espressif module for controlling face screen (aka oled) display buffer
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef COZMO_FACE_H_
#define COZMO_FACE_H_

#include "anki/types.h"
#include "clad/types/animationKeyFrames.h"

namespace Anki {
  namespace Cozmo {
    namespace Face {
      
      // Sets up data structures, call before any other methods
      Result Init();
      
      // Decompresses a new image from the basestation into the image buffer
      void Update(AnimKeyFrame::FaceImage img);
      
      // Shifts the image buffer to a new center
      void Move(s8 xCenter, s8 yCenter);
      
      // Blanks the screen momentarily
      void Blink();
      
      // Enables or disables periodic blinking
      void EnableBlink(bool enable);
      
      extern "C"
      {
        // Pump face buffer data out to OLED
        u32 PumpScreenData();
        
        // Display text on the screen until turned off
        void FacePrintf(const char *format, ...);
        
        // Return display to normal function
        void FaceUnPrintf(void);
      }
    } // Face
  } // Cozmo
} // Anki

#endif
