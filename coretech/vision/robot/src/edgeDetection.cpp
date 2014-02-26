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
    /*      enum State
    {
    ON_WHITE,
    ON_BLACK
    };*/

    // Float registers are more plentiful on the M4. Could this be faster?
    typedef f32 State;
    const f32 ON_WHITE = 0;
    const f32 ON_BLACK = 1.0f;

    NO_INLINE static void DetectBlurredEdges_horizontal(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists);
    NO_INLINE static void DetectBlurredEdges_vertical(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists);

    Result DetectBlurredEdges(const Array<u8> &image, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      Rectangle<s32> imageRegionOfInterest(0, image.get_size(1), 0, image.get_size(0));

      return DetectBlurredEdges(image, imageRegionOfInterest, grayvalueThreshold, minComponentWidth, everyNLines, edgeLists);
    }

    Result DetectBlurredEdges(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      AnkiConditionalErrorAndReturnValue(image.IsValid() && edgeLists.xDecreasing.IsValid() && edgeLists.xIncreasing.IsValid() && edgeLists.yDecreasing.IsValid() && edgeLists.yIncreasing.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "DetectBlurredEdges", "Arrays are not valid");

      AnkiConditionalErrorAndReturnValue(minComponentWidth > 0,
        RESULT_FAIL_INVALID_SIZE, "DetectBlurredEdges", "minComponentWidth is too small");

      AnkiConditionalErrorAndReturnValue(edgeLists.xDecreasing.get_maximumSize() == edgeLists.xIncreasing.get_maximumSize() &&
        edgeLists.xDecreasing.get_maximumSize() == edgeLists.yDecreasing.get_maximumSize() &&
        edgeLists.xDecreasing.get_maximumSize() == edgeLists.yIncreasing.get_maximumSize(),
        RESULT_FAIL_INVALID_SIZE, "DetectBlurredEdges", "All edgeLists must have the same maximum size");

      edgeLists.xDecreasing.Clear();
      edgeLists.xIncreasing.Clear();
      edgeLists.yDecreasing.Clear();
      edgeLists.yIncreasing.Clear();

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);
      const s32 imageStride = image.get_stride();

      edgeLists.imageHeight = imageHeight;
      edgeLists.imageWidth = imageWidth;

      // TODO: won't detect an edge on the last horizontal (for x search) or vertical (for y search)
      //       pixel. Is there a fast way to do this?

      DetectBlurredEdges_horizontal(image, imageRegionOfInterest, grayvalueThreshold, minComponentWidth, everyNLines, edgeLists);
      DetectBlurredEdges_vertical(image, imageRegionOfInterest, grayvalueThreshold, minComponentWidth, everyNLines, edgeLists);

      return RESULT_OK;
    } // Result DetectBlurredEdges()

    NO_INLINE static void DetectBlurredEdges_horizontal(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      //
      // Detect horizontal positive and negative transitions
      //

      const s32 imageStride = image.get_stride();

      s32 xDecreasingSize = edgeLists.xDecreasing.get_size();
      s32 xIncreasingSize = edgeLists.xIncreasing.get_size();
      s32 xMaxSizeM1 = edgeLists.xDecreasing.get_maximumSize() - 1;
      Point<s16> * restrict pXDecreasing = edgeLists.xDecreasing.Pointer(0);
      Point<s16> * restrict pXIncreasing = edgeLists.xIncreasing.Pointer(0);

      for(s32 y=imageRegionOfInterest.top; y<imageRegionOfInterest.bottom; y+=everyNLines) {
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

            bool foundEdge = false;

            //// Do non-word-aligned checks
            while(((x&3) != 0) && (x < imageRegionOfInterest.right)) {
              if(pImage[x] <= grayvalueThreshold) {
                foundEdge = true;
                break;
              }

              x++;
            }

            // If we didn't find an edge on the non-aligned checks
            if(!foundEdge) {
              // Do word-aligned checks 4x unrolled
              while(x < (imageRegionOfInterest.right-3)) {
                const u32 pixels_u8x4 = *reinterpret_cast<const u32 *>(pImage+x);

                if( (pixels_u8x4 & 0xFF) <= grayvalueThreshold ) {
                  foundEdge = true;
                  break;
                }

                x++;

                if( ((pixels_u8x4 & 0xFF00) >> 8) <= grayvalueThreshold ) {
                  foundEdge = true;
                  break;
                }

                x++;

                if( ((pixels_u8x4 & 0xFF0000) >> 16) <= grayvalueThreshold ) {
                  foundEdge = true;
                  break;
                }

                x++;

                if( ((pixels_u8x4 & 0xFF000000) >> 24) <= grayvalueThreshold ) {
                  foundEdge = true;
                  break;
                }

                x++;
                //x += 4;
              }

              // Finish reminder of checks
              if(!foundEdge) {
                while( (x < imageRegionOfInterest.right)) {
                  if(pImage[x] <= grayvalueThreshold) {
                    foundEdge = true;
                    break;
                  }
                  x++;
                }
              } // if(!foundEdge)
            } // if(!foundEdge)
            curState = ON_BLACK;

            //if(x < (imageRegionOfInterest.right-1)) {
            if(foundEdge) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                //edgeLists.xDecreasing.PushBack(Point<s16>(x,y));
                if(xDecreasingSize < xMaxSizeM1) {
                  pXDecreasing[xDecreasingSize].x = x;
                  pXDecreasing[xDecreasingSize].y = y;
                  xDecreasingSize++;
                }
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
                //edgeLists.xIncreasing.PushBack(Point<s16>(x,y));
                if(xIncreasingSize < xMaxSizeM1) {
                  pXIncreasing[xIncreasingSize].x = x;
                  pXIncreasing[xIncreasingSize].y = y;
                  xIncreasingSize++;
                }
              }

              lastSwitchX = x;
            } // if(x < (imageRegionOfInterest.right-1))
          } // if(curState == ON_WHITE) ... else

          x++;
        } // if(curState == ON_WHITE) ... else
      } // for(s32 y=0; y<imageRegionOfInterest.bottom; y++)

      edgeLists.xDecreasing.set_size(xDecreasingSize);
      edgeLists.xIncreasing.set_size(xIncreasingSize);
    } // DetectBlurredEdges_horizontal()

    NO_INLINE static void DetectBlurredEdges_vertical(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      //
      //  Detect vertical positive and negative transitions
      //

      const s32 imageStride = image.get_stride();

      s32 yDecreasingSize = edgeLists.yDecreasing.get_size();
      s32 yIncreasingSize = edgeLists.yIncreasing.get_size();
      s32 yMaxSizeM1 = edgeLists.yDecreasing.get_maximumSize() - 1;
      Point<s16> * restrict pYDecreasing = edgeLists.yDecreasing.Pointer(0);
      Point<s16> * restrict pYIncreasing = edgeLists.yIncreasing.Pointer(0);

      for(s32 x=imageRegionOfInterest.left; x<imageRegionOfInterest.right; x+=everyNLines) {
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
                //edgeLists.yDecreasing.PushBack(Point<s16>(x,y));
                if(yDecreasingSize < yMaxSizeM1) {
                  pYDecreasing[yDecreasingSize].x = x;
                  pYDecreasing[yDecreasingSize].y = y;
                  yDecreasingSize++;
                }
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
                //edgeLists.yIncreasing.PushBack(Point<s16>(x,y));
                if(yIncreasingSize < yMaxSizeM1) {
                  pYIncreasing[yIncreasingSize].x = x;
                  pYIncreasing[yIncreasingSize].y = y;
                  yIncreasingSize++;
                }
              }

              lastSwitchY = y;
            } // if(y < (imageRegionOfInterest.bottom-1)
          } // if(curState == ON_WHITE) ... else

          y++;
          pImage += imageStride;
        } // while(y < imageRegionOfInterest.bottom)
      } // for(s32 x=0; x<imageRegionOfInterest.right; x++)

      edgeLists.yDecreasing.set_size(yDecreasingSize);
      edgeLists.yIncreasing.set_size(yIncreasingSize);
    } // DetectBlurredEdges_vertical()
  } // namespace Embedded
} // namespace Anki
