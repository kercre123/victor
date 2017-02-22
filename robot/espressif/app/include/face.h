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
      static const int COLS = 128;
      static const int ROWS = 64;
      static const int PAGES = ROWS / 8;

      // Sets up data structures, call before any other methods
      Result Init();
      
      /// Returns the number of rects remaining to be transmitted
      int GetRemainingRects();
      
      /// Convert a regular number to 8 digits of BCD
      u32 DecToBCD(const u32 val);

      /// Clears all content from the face
      void Clear();

      /// Invalidates the screen state and forces it to start drawing blank
      void ResetScreen();

      /// Runs burn-in protection  (clears after long idle)
      void Update(void); 

      namespace Draw {
        void Clear(u64* frame);
        void Copy(u64* frame, const u64* image);
        bool Copy(u64* frame, const u32* image, const int cols, const int x, const int y);
        void Mask(u64* frame, const u64 mask);
        void Invert(u64* frame);
        bool Number(u64* frame, int digits, u32 value, int x, int y);
        bool NumberTiny(u64* frame, int digits, u32 value, int x, int y);
        bool Print(u64* frame, const char* text, const int characters, const int x, const int y);
        bool PrintSmall(u64* frame, const char* text, const int characters, int x, int y);
        //print `text` upto null or min(`characters`,DIGITDASH_WIFI_PSK_LEN) in PSK font
        bool PrintPsk(u64* frame, const char* text, const int characters, const int x, const int y);  
        void Flip(u64* frame);
      }

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
      } // Extern "C"

      // Suspend face operation and return a pointer to the face buffer
      s32 SuspendAndGetBuffer(void** buffer);

      // Return the buffer to the face and resume operation
      void ResumeAndRestoreBuffer();

    } // Face
  } // Cozmo
} // Anki

#endif
