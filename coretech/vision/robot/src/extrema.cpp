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

    Result ComputeLocalExtrema(const Array<u8> &image, const u8 grayvalueThreshold, const s32 minComponentWidth, FixedLengthList<Point<s16> > &xMinima, FixedLengthList<Point<s16> > &xMaxima, FixedLengthList<Point<s16> > &yMinima, FixedLengthList<Point<s16> > &yMaxima)
    {
      AnkiConditionalErrorAndReturnValue(image.IsValid() && xMinima.IsValid() && xMaxima.IsValid() && yMinima.IsValid() && yMaxima.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeLocalExtrema", "Arrays are not valid");

      AnkiConditionalErrorAndReturnValue(minComponentWidth > 0,
        RESULT_FAIL_INVALID_SIZE, "ComputeLocalExtrema", "minComponentWidth is too small");

      enum State
      {
        ON_WHITE,
        ON_BLACK
      };

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      //
      // Detect horizontal minima and maxima
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
            while( (x < imageWidth) && (pImage[x] > grayvalueThreshold)) {
              x++;
            }

            curState = ON_BLACK;

            if(x < (imageWidth-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                xMinima.PushBack(Point<s16>(x,y));
              }

              lastSwitchX = x;
            } // if(x < (imageWidth-1)
          } else { // if(curState == ON_WHITE)
            while( (x < imageWidth) && (pImage[x] < grayvalueThreshold)) {
              x++;
            }

            curState = ON_WHITE;

            if(x < (imageWidth-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                xMaxima.PushBack(Point<s16>(x,y));
              }

              lastSwitchX = x;
            } // if(x < (imageWidth-1))
          } // if(curState == ON_WHITE) ... else

          x++;
        } // if(curState == ON_WHITE) ... else
      } // for(s32 y=0; y<imageHeight; y++)

      //
      //  Detect vertical minima and maxima
      //

      for(s32 x=0; x<imageWidth; x++) {
        const u8 * restrict pImage = image.Pointer(0,x);

        State curState;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          curState = ON_WHITE;
        else
          curState = ON_BLACK;

        //s32 lastSwitchX = 0;
        //s32 x = 0;

        s32 lastSwitchY = 0;
        s32 y = 0;
        //while y <= imageHeight
        //  if curState == ON_WHITE
        //    while (y <= imageHeight) && (image(y,x) > grayvalueThreshold)
        //      y++;
        //end

        //  curState = 2;

        //if y ~= imageHeight
        //  componentWidth = y - lastSwitchY;

        //if componentWidth >= minComponentWidth
        //  yMinima1Image(y,x) = 1;
        //end

        //  lastSwitchY = y;
        //end
        //else
        //while (y <= imageHeight) && (image(y,x) < grayvalueThreshold)
        //  y++;
        //end

        //  curState = 1;

        //if y ~= imageHeight
        //  componentWidth = y - lastSwitchY;

        //if componentWidth >= minComponentWidth
        //  yMaxima1Image(y,x) = 1;
        //end

        //  lastSwitchY = y;
        //end
        //  end

        //  y++;
        //end
      } // for(s32 x=0; x<imageWidth; x++)

      return RESULT_OK;
    } // Result ComputeLocalExtrema()
  } // namespace Embedded
} // namespace Anki