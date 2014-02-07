/**
File: extrema.cpp
Author: Peter Barnum
Created: 2014-02-06

Computes local extrema from an image

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/miscVisionKernels.h"

namespace Anki
{
  namespace Embedded
  {
    static Result lastResult;

    Result DetectBlurredEdge(const Array<u8> &image, const u8 grayvalueThreshold, const s32 minComponentWidth, FixedLengthList<Point<s16> > &xDecreasing, FixedLengthList<Point<s16> > &xIncreasing, FixedLengthList<Point<s16> > &yDecreasing, FixedLengthList<Point<s16> > &yIncreasing)
    {
      AnkiConditionalErrorAndReturnValue(image.IsValid() && xDecreasing.IsValid() && xIncreasing.IsValid() && yDecreasing.IsValid() && yIncreasing.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "DetectBlurredEdge", "Arrays are not valid");

      AnkiConditionalErrorAndReturnValue(minComponentWidth > 0,
        RESULT_FAIL_INVALID_SIZE, "DetectBlurredEdge", "minComponentWidth is too small");

      enum State
      {
        ON_WHITE,
        ON_BLACK
      };

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);
      const s32 imageStride = image.get_stride();

      //
      // Detect horizontal minima and positive
      //

      for(s32 y=0; y<imageHeight; y++) {
        const u8 * restrict pImage = image.Pointer(y,0);

        State curState;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          curState = ON_WHITE;
        else
          curState = ON_BLACK;

        s32 lastSwitchX = 0;
        s32 x = 0;
        while(x < imageWidth) {
          if(curState == ON_WHITE) {
            // If on white

            while( (x < imageWidth) && (pImage[x] > grayvalueThreshold)) {
              x++;
            }

            curState = ON_BLACK;

            if(x < (imageWidth-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                xDecreasing.PushBack(Point<s16>(x,y));
              }

              lastSwitchX = x;
            } // if(x < (imageWidth-1)
          } else {
            // If on black

            while( (x < imageWidth) && (pImage[x] < grayvalueThreshold)) {
              x++;
            }

            curState = ON_WHITE;

            if(x < (imageWidth-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                xIncreasing.PushBack(Point<s16>(x,y));
              }

              lastSwitchX = x;
            } // if(x < (imageWidth-1))
          } // if(curState == ON_WHITE) ... else

          x++;
        } // if(curState == ON_WHITE) ... else
      } // for(s32 y=0; y<imageHeight; y++)

      //
      //  Detect vertical negative and maxima
      //

      for(s32 x=0; x<imageWidth; x++) {
        const u8 * restrict pImage = image.Pointer(0,x);

        State curState;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          curState = ON_WHITE;
        else
          curState = ON_BLACK;

        s32 lastSwitchY = 0;
        s32 y = 0;
        while(y < imageHeight) {
          if(curState == ON_WHITE) {
            // If on white

            while( (y < imageHeight) && (pImage[0] > grayvalueThreshold) ){
              y++;
              pImage += imageStride;
            }

            curState = ON_BLACK;

            if(y < (imageHeight-1)) {
              const s32 componentWidth = y - lastSwitchY;

              if(componentWidth >= minComponentWidth) {
                yDecreasing.PushBack(Point<s16>(x,y));
              }

              lastSwitchY = y;
            } // if(y < (imageHeight-1)
          } else {
            // If on black

            while( (y < imageHeight) && (pImage[0] < grayvalueThreshold) ) {
              y++;
              pImage += imageStride;
            }

            curState = ON_WHITE;

            if(y < (imageHeight-1)) {
              const s32 componentWidth = y - lastSwitchY;

              if(componentWidth >= minComponentWidth) {
                yIncreasing.PushBack(Point<s16>(x,y));
              }

              lastSwitchY = y;
            } // if(y < (imageHeight-1)
          } // if(curState == ON_WHITE) ... else

            y++;
            pImage += imageStride;
        } // while(y < imageHeight)
      } // for(s32 x=0; x<imageWidth; x++)

      return RESULT_OK;
    } // Result DetectBlurredEdge()
  } // namespace Embedded
} // namespace Anki