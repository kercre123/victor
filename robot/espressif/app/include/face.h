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
      
      /// Returns the number of rects remaining to be transmitted
      int GetRemainingRects();
      
      extern "C"
      {
        // Check for screen updates when the screen is idle
        void ManageScreen(void);
        
        // Display text on the screen until turned off
        void FacePrintf(const char *format, ...);
        
        // Return display to normal function
        void FaceUnPrintf(void);
        
        // Display debug text on the screen, overriding regular printf
        void FaceDebugPrintf(const char *format, ...);

        // Display a large font number on the screen
        void FaceDisplayNumber(int digits, u32 value, int x, int y);
      }
    } // Face
  } // Cozmo
} // Anki

#endif
