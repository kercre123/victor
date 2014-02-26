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
    //typedef f32 State;
    //const f32 ON_WHITE = 0;
    //const f32 ON_BLACK = 1.0f;

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

      //const Rectangle<u32> imageRegionOfInterestU32(imageRegionOfInterest.left, imageRegionOfInterest.right, imageRegionOfInterest.top, imageRegionOfInterest.bottom);

      //const u32 minComponentWidthU32 = minComponentWidth;

      s32 xDecreasingSize = edgeLists.xDecreasing.get_size();
      s32 xIncreasingSize = edgeLists.xIncreasing.get_size();
      s32 xMaxSizeM1 = edgeLists.xDecreasing.get_maximumSize() - 1;
      u32 * restrict pXDecreasing = reinterpret_cast<u32 *>(edgeLists.xDecreasing.Pointer(0));
      u32 * restrict pXIncreasing = reinterpret_cast<u32 *>(edgeLists.xIncreasing.Pointer(0));

      for(s32 y=imageRegionOfInterest.top; y<imageRegionOfInterest.bottom; y+=everyNLines) {
        const u8 * restrict pImage = image.Pointer(y,0);

        bool onWhite;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          onWhite = true;
        else
          onWhite = false;

        s32 lastSwitchX = imageRegionOfInterest.left;
        s32 x = imageRegionOfInterest.left;
        while(x < imageRegionOfInterest.right) {
          if(onWhite) {
            // If on white

            while(x < (imageRegionOfInterest.right-3)) {
              const u32 pixels_u8x4 = *reinterpret_cast<const u32 *>(pImage+x);

              if( (pixels_u8x4 & 0xFF) <= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF00) >> 8) <= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF0000) >> 16) <= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF000000) >> 24) <= grayvalueThreshold )
                break;

              x += 4;
            }

            while( (x < imageRegionOfInterest.right) && (pImage[x] > grayvalueThreshold)) {
              x++;
            }

            onWhite = false;

            if(x < (imageRegionOfInterest.right-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                // If there's room, add the point to the list
                if(xDecreasingSize < xMaxSizeM1) {
                  u32 newPoint = (y << 16) + x; // Set x and y in one operation
                  pXDecreasing[xDecreasingSize] = newPoint;

                  xDecreasingSize++;
                }
              }

              lastSwitchX = x;
            } // if(x < (imageRegionOfInterest.right-1)
          } else {
            // If on black

            while(x < (imageRegionOfInterest.right-3)) {
              const u32 pixels_u8x4 = *reinterpret_cast<const u32 *>(pImage+x);

              if( (pixels_u8x4 & 0xFF) >= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF00) >> 8) >= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF0000) >> 16) >= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF000000) >> 24) >= grayvalueThreshold )
                break;

              x += 4;
            }

            while( (x < imageRegionOfInterest.right) && (pImage[x] < grayvalueThreshold)) {
              x++;
            }

            onWhite = true;

            if(x < (imageRegionOfInterest.right-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                // If there's room, add the point to the list
                if(xIncreasingSize < xMaxSizeM1) {
                  u32 newPoint = (y << 16) + x;
                  pXIncreasing[xIncreasingSize] = newPoint;

                  xIncreasingSize++;
                }
              }

              lastSwitchX = x;
            } // if(x < (imageRegionOfInterest.right-1))
          } // if(onWhite) ... else

          x++;
        } // if(onWhite) ... else
      } // for(u32 y=0; y<imageRegionOfInterest.bottom; y++)

      edgeLists.xDecreasing.set_size(xDecreasingSize);
      edgeLists.xIncreasing.set_size(xIncreasingSize);
    } // DetectBlurredEdges_horizontal()

    NO_INLINE static void DetectBlurredEdges_vertical(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      //
      //  Detect vertical positive and negative transitions
      //

      const s32 imageStride = image.get_stride();

      //const Rectangle<u32> imageRegionOfInterestU32(imageRegionOfInterest.left, imageRegionOfInterest.right, imageRegionOfInterest.top, imageRegionOfInterest.bottom);

      //const u32 minComponentWidthU32 = minComponentWidth;

      s32 yDecreasingSize = edgeLists.yDecreasing.get_size();
      s32 yIncreasingSize = edgeLists.yIncreasing.get_size();
      s32 yMaxSizeM1 = edgeLists.yDecreasing.get_maximumSize() - 1;
      u32 * restrict pYDecreasing = reinterpret_cast<u32 *>(edgeLists.yDecreasing.Pointer(0));
      u32 * restrict pYIncreasing = reinterpret_cast<u32 *>(edgeLists.yIncreasing.Pointer(0));

      for(s32 x=imageRegionOfInterest.left; x<imageRegionOfInterest.right; x+=everyNLines) {
        const u8 * restrict pImage = image.Pointer(imageRegionOfInterest.top, x);

        bool onWhite;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          onWhite = true;
        else
          onWhite = false;

        s32 lastSwitchY = imageRegionOfInterest.top;
        s32 y = imageRegionOfInterest.top;
        while(y < imageRegionOfInterest.bottom) {
          if(onWhite) {
            // If on white

            while(y < (imageRegionOfInterest.bottom-1)) {
              const u8 pixel0 = pImage[0];
              const u8 pixel1 = pImage[imageStride];

              if(pixel0 <= grayvalueThreshold)
                break;

              if(pixel1 <= grayvalueThreshold)
                break;

              y += 2;
              pImage += 2*imageStride;
            }

            while( (y < imageRegionOfInterest.bottom) && (pImage[0] > grayvalueThreshold) ){
              y++;
              pImage += imageStride;
            }

            onWhite = false;

            if(y < (imageRegionOfInterest.bottom-1)) {
              const s32 componentWidth = y - lastSwitchY;

              if(componentWidth >= minComponentWidth) {
                // If there's room, add the point to the list
                if(yDecreasingSize < yMaxSizeM1) {
                  u32 newPoint = (y << 16) + x;
                  pYDecreasing[yDecreasingSize] = newPoint;

                  yDecreasingSize++;
                }
              }

              lastSwitchY = y;
            } // if(y < (imageRegionOfInterest.bottom-1)
          } else {
            // If on black

            while(y < (imageRegionOfInterest.bottom-1)) {
              const u8 pixel0 = pImage[0];
              const u8 pixel1 = pImage[imageStride];

              if(pixel0 >= grayvalueThreshold)
                break;

              if(pixel1 >= grayvalueThreshold)
                break;

              y += 2;
              pImage += 2*imageStride;
            }

            while( (y < imageRegionOfInterest.bottom) && (pImage[0] < grayvalueThreshold) ) {
              y++;
              pImage += imageStride;
            }

            onWhite = true;

            if(y < (imageRegionOfInterest.bottom-1)) {
              const s32 componentWidth = y - lastSwitchY;

              if(componentWidth >= minComponentWidth) {
                // If there's room, add the point to the list
                if(yIncreasingSize < yMaxSizeM1) {
                  u32 newPoint = (y << 16) + x;
                  pYIncreasing[yIncreasingSize] = newPoint;

                  yIncreasingSize++;
                }
              }

              lastSwitchY = y;
            } // if(y < (imageRegionOfInterest.bottom-1)
          } // if(onWhite) ... else

          y++;
          pImage += imageStride;
        } // while(y < imageRegionOfInterest.bottom)
      } // for(u32 x=0; x<imageRegionOfInterest.right; x++)

      edgeLists.yDecreasing.set_size(yDecreasingSize);
      edgeLists.yIncreasing.set_size(yIncreasingSize);
    } // DetectBlurredEdges_vertical()
  } // namespace Embedded
} // namespace Anki
