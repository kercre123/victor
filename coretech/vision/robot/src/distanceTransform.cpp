/**
File: distanceTransform.cpp
Author: Peter Barnum
Created: 2014-05-21

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/robot/comparisons.h"

#include "anki/vision/robot/imageProcessing.h"

#define USE_ARM_ACCELERATION

#if defined(__EDG__)
#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif
#else
#undef USE_ARM_ACCELERATION
#endif

#if defined(USE_ARM_ACCELERATION)
#ifdef USING_CHIP_SIMULATOR
#include <ARMCM4.h>
#else
#include <stm32f4xx.h>
#endif
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
      // 3x3 window, fixed-point, approximate Euclidian distance transform. Based on Borgefors "Distance transformations in digital images" 1986
      // Any pixel with grayvalue less than backgroundThreshold is treated as background
      Result DistanceTransform(const Array<u8> image, const u8 backgroundThreshold, FixedPointArray<s16> &distance)
      {
        const s32 numFractionalBits = distance.get_numFractionalBits();

        // For numFractionalBits==3, a==8 and b==11
        const s16 a = saturate_cast<s16>(0.95509 * pow(2.0,numFractionalBits));
        const s16 b = saturate_cast<s16>(1.3693 * pow(2.0,numFractionalBits));

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        // To prevent overflow, maxDistance is a bit less than s16_MAX
        const s16 maxDistance = s16_MAX - ( MAX(a,b) * (imageHeight + imageWidth) * (1<<numFractionalBits) );

        AnkiConditionalErrorAndReturnValue(AreValid(image, distance),
          RESULT_FAIL_INVALID_OBJECT, "DistanceTransform", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(image, distance),
          RESULT_FAIL_INVALID_SIZE, "DistanceTransform", "image and distance must be the same size");

        AnkiConditionalErrorAndReturnValue(numFractionalBits >= 0 && numFractionalBits < 15,
          RESULT_FAIL_INVALID_PARAMETER, "DistanceTransform", "numFractionalBits is out of range");

        //
        // Forward pass (left and down)
        //

        // Compute the left and top image edges (forward pass)
        {
          const u8 * restrict pImage = image.Pointer(0,0);

          s16 * restrict pDistance = distance.Pointer(0,0);

          // Top left
          if(pImage[0] < backgroundThreshold) {
            pDistance[0] = 0;
          } else {
            pDistance[0] = maxDistance;
          }

          // Left to right
          for(s32 x=1; x<imageWidth; x++) {
            if(pImage[x] < backgroundThreshold) {
              pDistance[x] = 0;
            } else {
              pDistance[x] = pDistance[x-1] + a;
            }
          }

          // Top to bottom
          for(s32 y=1; y<imageHeight; y++) {
            if(*image.Pointer(y,0) < backgroundThreshold) {
              *distance.Pointer(y,0) = 0;
            } else {
              *distance.Pointer(y,0) = *distance.Pointer(y-1,0) + a;
            }
          }
        } // Compute the left and top image edges (forward pass)

        // Compute the non left-right-top pixels (forward pass)
        {
          for(s32 y=1; y<imageHeight; y++) {
            const u8 * restrict pImage_y0 = image.Pointer(y,0);

            const s16 * restrict pDistance_ym1 = distance.Pointer(y-1,0);
            s16 * restrict pDistance_y0 = distance.Pointer(y,0);

            for(s32 x=1; x<imageWidth; x++) {
              if(pImage_y0[x] < backgroundThreshold) {
                pDistance_y0[x] = 0;
              } else {
                const s16 left = pDistance_y0[x-1] + a;
                const s16 leftUp = pDistance_ym1[x-1] + b;
                const s16 up = pDistance_ym1[x] + a;
                const s16 rightUp = pDistance_ym1[x+1] + b;

                pDistance_y0[x] = MIN(MIN(MIN(left, leftUp), up), rightUp);
              }
            }
          }
        } // Compute the non left-right-top pixels (forward pass)

        // Compute the right-edge pixels (forward pass)
        {
          const s32 x = imageWidth - 1;

          for(s32 y=1; y<imageHeight; y++) {
            const u8 * restrict pImage_y0 = image.Pointer(y,0);

            const s16 * restrict pDistance_ym1 = distance.Pointer(y-1,0);
            s16 * restrict pDistance_y0 = distance.Pointer(y,0);

            if(pImage_y0[x] < backgroundThreshold) {
              pDistance_y0[x] = 0;
            } else {
              const s16 left = pDistance_y0[x-1] + a;
              const s16 leftUp = pDistance_ym1[x-1] + b;
              const s16 up = pDistance_ym1[x] + a;

              pDistance_y0[x] = MIN(MIN(left, leftUp), up);
            }
          }
        } // Compute the right-edge pixels (forward pass)

        //
        // Backward pass (right and up)
        //

        // Compute the right and bottom image edges (backward pass)
        {
          s16 * restrict pDistance = distance.Pointer(imageHeight-1,0);

          // Right to left
          for(s32 x=imageWidth-2; x>=0; x--) {
            pDistance[x] = MIN(pDistance[x], pDistance[x+1] + a);
          }

          // Bottom to top
          for(s32 y=imageHeight-2; y>=0; y--) {
            *distance.Pointer(y,imageWidth-1) = MIN(*distance.Pointer(y,imageWidth-1), *distance.Pointer(y+1,imageWidth-1) + a);
          }
        } // Compute the right and bottom image edges (backward pass)

        // Compute the non left-right-top pixels (backward pass)
        {
          for(s32 y=imageHeight-2; y>=0; y--) {
            const s16 * restrict pDistance_yp1 = distance.Pointer(y+1,0);
            s16 * restrict pDistance_y0 = distance.Pointer(y,0);

            for(s32 x=imageWidth-2; x>0; x--) {
              const s16 center = pDistance_y0[x];
              if(center != 0) {
                const s16 right = pDistance_y0[x+1] + a;
                const s16 rightDown = pDistance_yp1[x+1] + b;
                const s16 down = pDistance_yp1[x] + a;
                const s16 leftDown = pDistance_yp1[x-1] + b;

                pDistance_y0[x] = MIN(MIN(MIN(MIN(right, rightDown), down), leftDown), center);
              }
            }
          }
        } // Compute the non left-right-top pixels (backward pass)

        // Compute the left edge (backward pass)
        {
          const s32 x = 0;

          for(s32 y=imageHeight-2; y>=0; y--) {
            const s16 * restrict pDistance_yp1 = distance.Pointer(y+1,0);
            s16 * restrict pDistance_y0 = distance.Pointer(y,0);

            const s16 center = pDistance_y0[x];
            if(center != 0) {
              const s16 right = pDistance_y0[x+1] + a;
              const s16 rightDown = pDistance_yp1[x+1] + b;
              const s16 down = pDistance_yp1[x] + a;

              pDistance_y0[x] = MIN(MIN(MIN(right, rightDown), down), center);
            }
          }
        } // Compute the left edge (backward pass)

        return RESULT_OK;
      }
    } // namespace ImageProcessing
  } // namespace Embedded
} // namespace Anki
