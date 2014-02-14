/**
File: edgeDetection.cpp
Author: Peter Barnum
Created: 2014-02-06

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/edgeDetection.h"

namespace Anki
{
  namespace Embedded
  {
    static Result lastResult;

    Result DetectBlurredEdges(const Array<u8> &image, const u8 grayvalueThreshold, const s32 minComponentWidth, EdgeLists &edgeLists)
    {
      Rectangle<s32> imageRegionOfInterest(0, image.get_size(1), 0, image.get_size(0));

      return DetectBlurredEdges(image, imageRegionOfInterest, grayvalueThreshold, minComponentWidth, edgeLists);
    }

    Result DetectBlurredEdges(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, EdgeLists &edgeLists)
    {
      AnkiConditionalErrorAndReturnValue(image.IsValid() && edgeLists.xDecreasing.IsValid() && edgeLists.xIncreasing.IsValid() && edgeLists.yDecreasing.IsValid() && edgeLists.yIncreasing.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "DetectBlurredEdges", "Arrays are not valid");

      AnkiConditionalErrorAndReturnValue(minComponentWidth > 0,
        RESULT_FAIL_INVALID_SIZE, "DetectBlurredEdges", "minComponentWidth is too small");

      enum State
      {
        ON_WHITE,
        ON_BLACK
      };

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);
      const s32 imageStride = image.get_stride();

      edgeLists.imageHeight = imageHeight;
      edgeLists.imageWidth = imageWidth;

      // TODO: won't detect an edge on the last horizontal (for x search) or vertical (for y search)
      //       pixel. Is there a fast way to do this?

      //
      // Detect horizontal positive and negative transitions
      //

      for(s32 y=imageRegionOfInterest.top; y<imageRegionOfInterest.bottom; y++) {
        const u8 * restrict pImage = image.Pointer(y,0);

        State curState;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          curState = ON_WHITE;
        else
          curState = ON_BLACK;

        s32 lastSwitchX = imageRegionOfInterest.left;
        s32 x = imageRegionOfInterest.left;
        while(x < imageRegionOfInterest.right) {
          if(curState == ON_WHITE) {
            // If on white

            while( (x < imageRegionOfInterest.right) && (pImage[x] > grayvalueThreshold)) {
              x++;
            }

            curState = ON_BLACK;

            if(x < (imageRegionOfInterest.right-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                edgeLists.xDecreasing.PushBack(Point<s16>(x,y));
              }

              lastSwitchX = x;
            } // if(x < (imageRegionOfInterest.right-1)
          } else {
            // If on black

            while( (x < imageRegionOfInterest.right) && (pImage[x] < grayvalueThreshold)) {
              x++;
            }

            curState = ON_WHITE;

            if(x < (imageRegionOfInterest.right-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                edgeLists.xIncreasing.PushBack(Point<s16>(x,y));
              }

              lastSwitchX = x;
            } // if(x < (imageRegionOfInterest.right-1))
          } // if(curState == ON_WHITE) ... else

          x++;
        } // if(curState == ON_WHITE) ... else
      } // for(s32 y=0; y<imageRegionOfInterest.bottom; y++)

      //
      //  Detect vertical positive and negative transitions
      //

      for(s32 x=imageRegionOfInterest.left; x<imageRegionOfInterest.right; x++) {
        const u8 * restrict pImage = image.Pointer(imageRegionOfInterest.top, x);

        State curState;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          curState = ON_WHITE;
        else
          curState = ON_BLACK;

        s32 lastSwitchY = imageRegionOfInterest.top;
        s32 y = imageRegionOfInterest.top;
        while(y < imageRegionOfInterest.bottom) {
          if(curState == ON_WHITE) {
            // If on white

            while( (y < imageRegionOfInterest.bottom) && (pImage[0] > grayvalueThreshold) ){
              y++;
              pImage += imageStride;
            }

            curState = ON_BLACK;

            if(y < (imageRegionOfInterest.bottom-1)) {
              const s32 componentWidth = y - lastSwitchY;

              if(componentWidth >= minComponentWidth) {
                edgeLists.yDecreasing.PushBack(Point<s16>(x,y));
              }

              lastSwitchY = y;
            } // if(y < (imageRegionOfInterest.bottom-1)
          } else {
            // If on black

            while( (y < imageRegionOfInterest.bottom) && (pImage[0] < grayvalueThreshold) ) {
              y++;
              pImage += imageStride;
            }

            curState = ON_WHITE;

            if(y < (imageRegionOfInterest.bottom-1)) {
              const s32 componentWidth = y - lastSwitchY;

              if(componentWidth >= minComponentWidth) {
                edgeLists.yIncreasing.PushBack(Point<s16>(x,y));
              }

              lastSwitchY = y;
            } // if(y < (imageRegionOfInterest.bottom-1)
          } // if(curState == ON_WHITE) ... else

          y++;
          pImage += imageStride;
        } // while(y < imageRegionOfInterest.bottom)
      } // for(s32 x=0; x<imageRegionOfInterest.right; x++)

      return RESULT_OK;
    } // Result DetectBlurredEdges()
  } // namespace Embedded
} // namespace Anki