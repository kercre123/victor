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
        BeginBenchmark("dt_init");

        const s32 numFractionalBits = distance.get_numFractionalBits();

        // For numFractionalBits==3, a==8 and b==11
        const u16 a = saturate_cast<u16>(0.95509 * pow(2.0,numFractionalBits));
        const u16 b = saturate_cast<u16>(1.3693 * pow(2.0,numFractionalBits));

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        const u32 backgroundThresholdX2 = static_cast<u32>(backgroundThreshold) | (static_cast<u32>(backgroundThreshold) << 16);
        const u32 aX2 = static_cast<u32>(a) | (static_cast<u32>(a) << 16);
        const u32 bX2 = static_cast<u32>(b) | (static_cast<u32>(b) << 16);

        // To prevent overflow, maxDistance is a bit less than s16_MAX
        const u16 maxDistance = s16_MAX - ( MAX(a,b) * (imageHeight + imageWidth) * (1<<numFractionalBits) );

        AnkiConditionalErrorAndReturnValue(AreValid(image, distance),
          RESULT_FAIL_INVALID_OBJECT, "DistanceTransform", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(image, distance),
          RESULT_FAIL_INVALID_SIZE, "DistanceTransform", "image and distance must be the same size");

        AnkiConditionalErrorAndReturnValue(numFractionalBits >= 0 && numFractionalBits < 15,
          RESULT_FAIL_INVALID_PARAMETER, "DistanceTransform", "numFractionalBits is out of range");

        AnkiConditionalErrorAndReturnValue(imageWidth%4 == 0,
          RESULT_FAIL_INVALID_SIZE, "FastGradient", "Image width must be divisible by 4");

        EndBenchmark("dt_init");

        //
        // Forward pass (left and down)
        //

        BeginBenchmark("dt_forward1");

        // Compute the left and top image edges (forward pass)
        {
          const u8 * restrict pImage = image.Pointer(0,0);

          u16 * restrict pDistance = reinterpret_cast<u16*>( distance.Pointer(0,0) );

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

        EndBenchmark("dt_forward1");

        BeginBenchmark("dt_forward2");

        // Compute the non left-top pixels (forward pass)
        {
          for(s32 y=1; y<imageHeight; y++) {
            const u8 * restrict pImage_y0 = image.Pointer(y,0);

            const u16 * restrict pDistance_ym1 = reinterpret_cast<u16*>( distance.Pointer(y-1,0) );
            u16 * restrict pDistance_y0 = reinterpret_cast<u16*>( distance.Pointer(y,0) );

            //for(s32 x=1; x<imageWidth; x++) {
            for(s32 x=1; x<4; x++) {
              if(pImage_y0[x] < backgroundThreshold) {
                pDistance_y0[x] = 0;
              } else {
                const u16 left = pDistance_y0[x-1] + a;
                const u16 leftUp = pDistance_ym1[x-1] + b;
                const u16 up = pDistance_ym1[x] + a;
                const u16 rightUp = pDistance_ym1[x+1] + b;

                pDistance_y0[x] = MIN(MIN(MIN(left, leftUp), up), rightUp);
              }
            }

            // TODO: If all of the image pixels are background, set zero without doing the computation

            // First SIMD the top-left, top, top-right
            for(s32 x=4; x<imageWidth-3; x+=4) {
              const u32 leftUp10  = *reinterpret_cast<const u32*>(pDistance_ym1 + x - 1) + bX2;
              const u32 leftUp32  = *reinterpret_cast<const u32*>(pDistance_ym1 + x + 1) + bX2;

              const u32 up10      = *reinterpret_cast<const u32*>(pDistance_ym1 + x)     + aX2;
              const u32 up32      = *reinterpret_cast<const u32*>(pDistance_ym1 + x + 2) + aX2;

              const u32 rightUp10 = *reinterpret_cast<const u32*>(pDistance_ym1 + x + 1) + bX2;
              const u32 rightUp32 = *reinterpret_cast<const u32*>(pDistance_ym1 + x + 3) + bX2;

              u32 distance10 = leftUp10;
              u32 distance32 = leftUp32;

              // Compute the minimum of left, up, and right
#if !defined(USE_ARM_ACCELERATION)
              if( (up10 & 0xFFFF) < (distance10 & 0xFFFF)) {
                distance10 &= 0xFFFF0000;
                distance10 |= up10 & 0xFFFF;
              }

              if( (up10 & 0xFFFF0000) < (distance10 & 0xFFFF0000)) {
                distance10 &= 0x0000FFFF;
                distance10 |= up10 & 0xFFFF0000;
              }

              if( (up32 & 0xFFFF) < (distance32 & 0xFFFF)) {
                distance32 &= 0xFFFF0000;
                distance32 |= up32 & 0xFFFF;
              }

              if( (up32 & 0xFFFF0000) < (distance32 & 0xFFFF0000)) {
                distance32 &= 0x0000FFFF;
                distance32 |= up32 & 0xFFFF0000;
              }

              if( (rightUp10 & 0xFFFF) < (distance10 & 0xFFFF)) {
                distance10 &= 0xFFFF0000;
                distance10 |= rightUp10 & 0xFFFF;
              }

              if( (rightUp10 & 0xFFFF0000) < (distance10 & 0xFFFF0000)) {
                distance10 &= 0x0000FFFF;
                distance10 |= rightUp10 & 0xFFFF0000;
              }

              if( (rightUp32 & 0xFFFF) < (distance32 & 0xFFFF)) {
                distance32 &= 0xFFFF0000;
                distance32 |= rightUp32 & 0xFFFF;
              }

              if( (rightUp32 & 0xFFFF0000) < (distance32 & 0xFFFF0000)) {
                distance32 &= 0x0000FFFF;
                distance32 |= rightUp32 & 0xFFFF0000;
              }

#else // #if !defined(USE_ARM_ACCELERATION)
              __USUB16(leftUp10, distance10);
              distance10 = __SEL(distance10, leftUp10);

              __USUB16(leftUp32, distance32);
              distance32 = __SEL(distance32, leftUp32);

              __USUB16(up10, distance10);
              distance10 = __SEL(distance10, up10);

              __USUB16(up32, distance32);
              distance32 = __SEL(distance32, up32);

              __USUB16(rightUp10, distance10);
              distance10 = __SEL(distance10, rightUp10);

              __USUB16(rightUp32, distance32);
              distance32 = __SEL(distance32, rightUp32);
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

              *reinterpret_cast<u32*>(pDistance_y0 + x) = distance10;
              *reinterpret_cast<u32*>(pDistance_y0 + x + 2) = distance32;
            } // for(s32 x=4; x<imageWidth-3; x+=4)

            // Second SIMD the left
            for(s32 x=4; x<imageWidth-3; x+=4) {
              const u32 image3210 = *reinterpret_cast<const u32*>(pImage_y0 + x);

              const u32 image10 = (image3210 & 0xFF) | ((image3210 & 0xFF00) << 8);
              const u32 image32 = ((image3210 & 0xFF0000) >> 16) | ((image3210 & 0xFF000000) >> 8);

              const u32 previousDistance = pDistance_y0[x-1];

              u32 distance10 = *reinterpret_cast<u32*>(pDistance_y0 + x);
              u32 distance32 = *reinterpret_cast<u32*>(pDistance_y0 + x + 2);

              //#if !defined(USE_ARM_ACCELERATION)

              // If any pixels are zero, set the distance to zero
              // Otherwise, each is the min of its "left + a" and its current value
              if( (image10 & 0xFF) < backgroundThreshold) {
                distance10 &= 0xFFFF0000;
              } else {
                const u32 minDistance0 = MIN(distance10 & 0xFFFF, previousDistance + a);
                distance10 &= 0xFFFF0000;
                distance10 |= minDistance0;
              }

              if( (image10 & 0xFF0000) < (static_cast<u32>(backgroundThreshold)<<16)) {
                distance10 &= 0x0000FFFF;
              } else {
                const u32 minDistance1 = MIN(distance10 & 0xFFFF0000, ((distance10 & 0x0000FFFF) + a) << 16);
                distance10 &= 0x0000FFFF;
                distance10 |= minDistance1;
              }

              if( (image32 & 0xFF) < backgroundThreshold) {
                distance32 &= 0xFFFF0000;
              } else {
                const u32 minDistance2 = MIN(distance32 & 0xFFFF, ((distance10 & 0xFFFF0000) >> 16) + a);
                distance32 &= 0xFFFF0000;
                distance32 |= minDistance2;
              }

              if( (image32 & 0xFF0000) < (static_cast<u32>(backgroundThreshold)<<16)) {
                distance32 &= 0x0000FFFF;
              } else {
                const u32 minDistance3 = MIN(distance32 & 0xFFFF0000, ((distance32 & 0x0000FFFF) + a) << 16);
                distance32 &= 0x0000FFFF;
                distance32 |= minDistance3;
              }

              //#else // #if !defined(USE_ARM_ACCELERATION)
              //#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

              *reinterpret_cast<u32*>(pDistance_y0 + x) = distance10;
              *reinterpret_cast<u32*>(pDistance_y0 + x + 2) = distance32;
            } // for(s32 x=4; x<imageWidth-3; x+=4)

            // TODO: If all of the image pixels are background, set zero without doing the computation

            // First do the top-left, top, top-right
            for(s32 x=4; x<imageWidth-3; x+=4) {
              const u32 leftUp10  = *reinterpret_cast<const u32*>(pDistance_ym1 + x - 1) + bX2;
              const u32 leftUp32  = *reinterpret_cast<const u32*>(pDistance_ym1 + x + 1) + bX2;

              const u32 up10      = *reinterpret_cast<const u32*>(pDistance_ym1 + x)     + aX2;
              const u32 up32      = *reinterpret_cast<const u32*>(pDistance_ym1 + x + 2) + aX2;

              const u32 rightUp10 = *reinterpret_cast<const u32*>(pDistance_ym1 + x + 1) + bX2;
              const u32 rightUp32 = *reinterpret_cast<const u32*>(pDistance_ym1 + x + 3) + bX2;

              //#if !defined(USE_ARM_ACCELERATION)

              // Compute the minimum of left, up, and right
              u32 distance10 = leftUp10;
              u32 distance32 = leftUp32;

              if( (up10 & 0xFFFF) < (distance10 & 0xFFFF)) {
                distance10 &= 0xFFFF0000;
                distance10 |= up10 & 0xFFFF;
              }

              if( (up10 & 0xFFFF0000) < (distance10 & 0xFFFF0000)) {
                distance10 &= 0x0000FFFF;
                distance10 |= up10 & 0xFFFF0000;
              }

              if( (up32 & 0xFFFF) < (distance32 & 0xFFFF)) {
                distance32 &= 0xFFFF0000;
                distance32 |= up32 & 0xFFFF;
              }

              if( (up32 & 0xFFFF0000) < (distance32 & 0xFFFF0000)) {
                distance32 &= 0x0000FFFF;
                distance32 |= up32 & 0xFFFF0000;
              }

              if( (rightUp10 & 0xFFFF) < (distance10 & 0xFFFF)) {
                distance10 &= 0xFFFF0000;
                distance10 |= rightUp10 & 0xFFFF;
              }

              if( (rightUp10 & 0xFFFF0000) < (distance10 & 0xFFFF0000)) {
                distance10 &= 0x0000FFFF;
                distance10 |= rightUp10 & 0xFFFF0000;
              }

              if( (rightUp32 & 0xFFFF) < (distance32 & 0xFFFF)) {
                distance32 &= 0xFFFF0000;
                distance32 |= rightUp32 & 0xFFFF;
              }

              if( (rightUp32 & 0xFFFF0000) < (distance32 & 0xFFFF0000)) {
                distance32 &= 0x0000FFFF;
                distance32 |= rightUp32 & 0xFFFF0000;
              }

              *reinterpret_cast<u32*>(pDistance_y0 + x) = distance10;
              *reinterpret_cast<u32*>(pDistance_y0 + x + 2) = distance32;

              //#else // #if !defined(USE_ARM_ACCELERATION)
              //#endif // #if !defined(USE_ARM_ACCELERATION) ... #else
            } // for(s32 x=4; x<imageWidth-3; x+=4)

            // Second do left

            for(s32 x=4; x<imageWidth-3; x+=4) {
              const u32 image3210 = *reinterpret_cast<const u32*>(pImage_y0 + x);

              const u32 image10 = (image3210 & 0xFF) | ((image3210 & 0xFF00) << 8);
              const u32 image32 = ((image3210 & 0xFF0000) >> 16) | ((image3210 & 0xFF000000) >> 8);

              const u32 previousDistance = pDistance_y0[x-1];

              u32 distance10 = *reinterpret_cast<u32*>(pDistance_y0 + x);
              u32 distance32 = *reinterpret_cast<u32*>(pDistance_y0 + x + 2);

              //#if !defined(USE_ARM_ACCELERATION)

              // If any pixels are zero, set the distance to zero
              if( (image10 & 0xFF) < backgroundThreshold) {
                distance10 &= 0xFFFF0000;
              } else {
                const u32 minDistance0 = MIN(distance10 & 0xFFFF, previousDistance + a);
                distance10 &= 0xFFFF0000;
                distance10 |= minDistance0;
              }

              if( (image10 & 0xFF0000) < (static_cast<u32>(backgroundThreshold)<<16)) {
                distance10 &= 0x0000FFFF;
              } else {
                const u32 minDistance1 = MIN(distance10 & 0xFFFF0000, ((distance10 & 0x0000FFFF) + a) << 16);
                distance10 &= 0x0000FFFF;
                distance10 |= minDistance1;
              }

              if( (image32 & 0xFF) < backgroundThreshold) {
                distance32 &= 0xFFFF0000;
              } else {
                const u32 minDistance2 = MIN(distance32 & 0xFFFF, ((distance10 & 0xFFFF0000) >> 16) + a);
                distance32 &= 0xFFFF0000;
                distance32 |= minDistance2;
              }

              if( (image32 & 0xFF0000) < (static_cast<u32>(backgroundThreshold)<<16)) {
                distance32 &= 0x0000FFFF;
              } else {
                const u32 minDistance3 = MIN(distance32 & 0xFFFF0000, ((distance32 & 0x0000FFFF) + a) << 16);
                distance32 &= 0x0000FFFF;
                distance32 |= minDistance3;
              }

              //#else // #if !defined(USE_ARM_ACCELERATION)
              //#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

              *reinterpret_cast<u32*>(pDistance_y0 + x) = distance10;
              *reinterpret_cast<u32*>(pDistance_y0 + x + 2) = distance32;
            } // for(s32 x=4; x<imageWidth-3; x+=4)

            // Compute the right-edge pixels (forward pass)
            {
              const s32 x = imageWidth - 1;

              if(pImage_y0[x] < backgroundThreshold) {
                pDistance_y0[x] = 0;
              } else {
                const u16 left = pDistance_y0[x-1] + a;
                const u16 leftUp = pDistance_ym1[x-1] + b;
                const u16 up = pDistance_ym1[x] + a;

                pDistance_y0[x] = MIN(MIN(left, leftUp), up);
              }
            } // Compute the right-edge pixels (forward pass)
          } // for(s32 y=1; y<imageHeight; y++)
        } // Compute the non left-right-top pixels (forward pass)

        EndBenchmark("dt_forward2");

        //distance.Print("distance");

        //
        // Backward pass (right and up)
        //

        BeginBenchmark("dt_backward1");

        // Compute the right and bottom image edges (backward pass)
        {
          u16 * restrict pDistance = reinterpret_cast<u16*>( distance.Pointer(imageHeight-1,0) );

          // Right to left
          for(s32 x=imageWidth-2; x>=0; x--) {
            pDistance[x] = MIN(pDistance[x], pDistance[x+1] + a);
          }

          // Bottom to top
          for(s32 y=imageHeight-2; y>=0; y--) {
            *distance.Pointer(y,imageWidth-1) = MIN(*distance.Pointer(y,imageWidth-1), *distance.Pointer(y+1,imageWidth-1) + a);
          }
        } // Compute the right and bottom image edges (backward pass)

        EndBenchmark("dt_backward1");

        BeginBenchmark("dt_backward2");

        // Compute the non right-top pixels (backward pass)
        {
          for(s32 y=imageHeight-2; y>=0; y--) {
            const u16 * restrict pDistance_yp1 = reinterpret_cast<u16*>( distance.Pointer(y+1,0) );
            u16 * restrict pDistance_y0 = reinterpret_cast<u16*>( distance.Pointer(y,0) );

            for(s32 x=imageWidth-2; x>0; x--) {
              const u16 center = pDistance_y0[x];
              if(center != 0) {
                const u16 right = pDistance_y0[x+1] + a;
                const u16 rightDown = pDistance_yp1[x+1] + b;
                const u16 down = pDistance_yp1[x] + a;
                const u16 leftDown = pDistance_yp1[x-1] + b;

                pDistance_y0[x] = MIN(MIN(MIN(MIN(right, rightDown), down), leftDown), center);
              }
            }

            // Compute the left edge (backward pass)
            {
              const s32 x = 0;

              const u16 center = pDistance_y0[x];
              if(center != 0) {
                const u16 right = pDistance_y0[x+1] + a;
                const u16 rightDown = pDistance_yp1[x+1] + b;
                const u16 down = pDistance_yp1[x] + a;

                pDistance_y0[x] = MIN(MIN(MIN(right, rightDown), down), center);
              }
            } // Compute the left edge (backward pass)
          } // for(s32 y=imageHeight-2; y>=0; y--)
        } // Compute the non left-right-top pixels (backward pass)

        EndBenchmark("dt_backward2");

        return RESULT_OK;
      }
    } // namespace ImageProcessing
  } // namespace Embedded
} // namespace Anki
