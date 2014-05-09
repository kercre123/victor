/**
File: computeCharacteristicScale.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/utilities.h"
#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/interpolate.h"

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/integralImage.h"
#include "anki/vision/robot/imageProcessing.h"

//#define HAVE_64_BIT_ARITHMETIC

namespace Anki
{
  namespace Embedded
  {
#define MAX_PYRAMID_LEVELS 8
#define MAX_ALPHAS 128

    // These are not inlined, to make it easier to hand-optimize them. Inlining them will probably only slightly increase speed.
    NO_INLINE void ecvcs_filterRows(const ScrollingIntegralImage_u8_s32 &integralImage, const s32 scaleImage_numPyramidLevels, const s32 imageY, const s32 imageWidth, Array<u8> * restrict filteredRows);
    NO_INLINE void ecvcs_computeBinaryImage_numPyramids3(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow);
    NO_INLINE void ecvcs_computeBinaryImage(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow);

    NO_INLINE void ecvcs_filterRows(const ScrollingIntegralImage_u8_s32 &integralImage, const s32 scaleImage_numPyramidLevels, const s32 imageY, const s32 imageWidth, Array<u8> * restrict filteredRows)
    {
      // To normalize a sum of 1 / ((2*n+1)^2), we approximate a division as a mulitiply and shift.
      // These multiply coefficients were computed in matlab by typing:
      // " for i=1:5 disp(sprintf('i:%d',i')); computeClosestMultiplicationShift((2*(2^i)+1)^2, 31, 8); disp(sprintf('\n')); end; "
      const s32 normalizationMultiply[5] = {41, 101, 227, 241, 31}; // For halfWidths in [1,5]
      const s32 normalizationBitShifts[5] = {10, 13, 16, 18, 17};

      // For the given row of the image, compute the blurred version for each level of the pyramid
      for(s32 pyramidLevel=0; pyramidLevel<=scaleImage_numPyramidLevels; pyramidLevel++) {
        //BeginBenchmark("ecvcs_filterRows_init");
        const s32 filterHalfWidth = 1 << (pyramidLevel+1); // filterHalfWidth = 2^pyramidLevel;
        const Rectangle<s16> filter(-filterHalfWidth, filterHalfWidth, -filterHalfWidth, filterHalfWidth);

        const s32 multiply = normalizationMultiply[pyramidLevel];
        const s32 shift = normalizationBitShifts[pyramidLevel];

        //EndBenchmark("ecvcs_filterRows_init");

        //BeginBenchmark("ecvcs_filterRows_filter&normalize");

        // Compute the sums using the integral image
        integralImage.FilterRow(filter, imageY, multiply, shift, filteredRows[pyramidLevel]);

        //EndBenchmark("ecvcs_filterRows_filter&normalize");
      } // for(s32 pyramidLevel=0; pyramidLevel<=numLevels; pyramidLevel++)
    } // staticInline ecvcs_filterRows()

    NO_INLINE void ecvcs_computeBinaryImage_numPyramids3(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow)
    {
      const s32 thresholdMultiplier_numFractionalBits = 16;

      const u8 * restrict pImage = image[imageY];

      const u8 * restrict pFilteredRows0 = filteredRows[0][0];
      const u8 * restrict pFilteredRows1 = filteredRows[1][0];
      const u8 * restrict pFilteredRows2 = filteredRows[2][0];
      const u8 * restrict pFilteredRows3 = filteredRows[3][0];

      for(s32 x=imageWidth-1; x>=0; x--) {
        s32 scaleValue;

        //for(s32 pyramidLevel=0; pyramidLevel<3; pyramidLevel++) {
        const s32 dog0 = ABS(static_cast<s32>(pFilteredRows1[x]) - static_cast<s32>(pFilteredRows0[x]));
        const s32 dog1 = ABS(static_cast<s32>(pFilteredRows2[x]) - static_cast<s32>(pFilteredRows1[x]));
        const s32 dog2 = ABS(static_cast<s32>(pFilteredRows3[x]) - static_cast<s32>(pFilteredRows2[x]));

        if(dog0 > dog1) {
          if(dog0 > dog2) {
            scaleValue = pFilteredRows1[x];
          } else {
            scaleValue = pFilteredRows3[x];
          }
        } else if(dog1 > dog2) {
          scaleValue = pFilteredRows2[x];
        } else {
          scaleValue = pFilteredRows3[x];
        }

        //} // for(s32 pyramidLevel=0; pyramidLevel<3; scaleImage_numPyramidLevels++)

        const s32 thresholdValue = (scaleValue*scaleImage_thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;
        if(pImage[x] < thresholdValue) {
          pBinaryImageRow[x] = 1;
        } else {
          pBinaryImageRow[x] = 0;
        }
      } // for(s32 x=0; x<imageWidth; x++)
    } // staticInline void ecvcs_computeBinaryImage()

    NO_INLINE void ecvcs_computeBinaryImage(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow)
    {
      const s32 thresholdMultiplier_numFractionalBits = 16;

      const u8 * restrict pImage = image[imageY];

      const u8 * restrict pFilteredRows[5];
      for(s32 pyramidLevel=0; pyramidLevel<=scaleImage_numPyramidLevels; pyramidLevel++) {
        pFilteredRows[pyramidLevel] = filteredRows[pyramidLevel][0];
      }

      for(s32 x=0; x<imageWidth; x++) {
        s32 scaleValue = -1;
        s32 dogMax = s32_MIN;
        for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; pyramidLevel++) {
          const s32 dog = ABS(static_cast<s32>(pFilteredRows[pyramidLevel+1][x]) - static_cast<s32>(pFilteredRows[pyramidLevel][x]));

          if(dog > dogMax) {
            dogMax = dog;
            scaleValue = pFilteredRows[pyramidLevel+1][x];
          } // if(dog > dogMax)
        } // for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; scaleImage_numPyramidLevels++)

        const s32 thresholdValue = (scaleValue*scaleImage_thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;
        if(pImage[x] < thresholdValue) {
          pBinaryImageRow[x] = 1;
        } else {
          pBinaryImageRow[x] = 0;
        }
      } // for(s32 x=0; x<imageWidth; x++)
    } // staticInline void ecvcs_computeBinaryImage()

    Result ExtractComponentsViaCharacteristicScale(
      const Array<u8> &image,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack fastScratch,
      MemoryStack slowScratch)
    {
      BeginBenchmark("ecvcs_init");

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      // Integral image constants
      s32 numBorderPixels;
      s32 integralImageHeight;

      if(imageWidth <= 320 && scaleImage_numPyramidLevels <= 3) {
        numBorderPixels = 17; // For qvga, 2^4 + 1 = 17
        integralImageHeight = 64; // 96*(640+33*2)*4 = 271104, though with padding, it is 96*720*4 = 276480
      } else {
        // Note: These numbers are liable to be too big to fit on the M4 efficiently
        s32 scaleFactor = static_cast<s32>(ceilf(static_cast<f32>(imageWidth) / 320.0f));
        numBorderPixels = (1<<(scaleImage_numPyramidLevels+1)) + 1;
        integralImageHeight = 64*scaleFactor;
      }

      const s32 numRowsToScroll = integralImageHeight - 2*numBorderPixels;

      const s32 maxFilterHalfWidth = 1 << (scaleImage_numPyramidLevels+1);

      Result lastResult;

      AnkiConditionalErrorAndReturnValue(fastScratch.IsValid() && slowScratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(fastScratch.get_buffer() != slowScratch.get_buffer(),
        RESULT_FAIL_ALIASED_MEMORY, "ExtractComponentsViaCharacteristicScale", "fast and slow scratch buffers cannot be the same object");

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale", "image is not valid");

      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale", "components is not valid");

      //AnkiConditionalErrorAndReturnValue(scaleImage_numPyramidLevels <= 4 && scaleImage_numPyramidLevels > 0,
      //  RESULT_FAIL_INVALID_PARAMETER, "ExtractComponentsViaCharacteristicScale", "scaleImage_numPyramidLevels must be less than %d", 4+1);

      AnkiConditionalErrorAndReturnValue(scaleImage_numPyramidLevels <= 8 && scaleImage_numPyramidLevels > 0,
        RESULT_FAIL_INVALID_PARAMETER, "ExtractComponentsViaCharacteristicScale", "scaleImage_numPyramidLevels must be less than %d", 8+1);

      // Initialize the first integralImageHeight rows of the integralImage
      ScrollingIntegralImage_u8_s32 integralImage(integralImageHeight, imageWidth, numBorderPixels, slowScratch);
      if((lastResult = integralImage.ScrollDown(image, integralImageHeight, fastScratch)) != RESULT_OK)
        return lastResult;

      // Prepare the memory for the filtered rows for each level of the pyramid
      Array<u8> filteredRows[9];
      for(s32 i=0; i<=scaleImage_numPyramidLevels; i++) {
        filteredRows[i] = Array<u8>(1, imageWidth, fastScratch);
      }

      Array<u8> binaryImageRow(1, imageWidth, fastScratch);
      u8 * restrict pBinaryImageRow = binaryImageRow[0];

      AnkiConditionalErrorAndReturnValue(binaryImageRow.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "ExtractComponentsViaCharacteristicScale", "binaryImageRow is not valid");

      if((lastResult = components.Extract2dComponents_PerRow_Initialize(fastScratch, slowScratch)) != RESULT_OK)
        return lastResult;

      s32 imageY = 0;

      EndBenchmark("ecvcs_init");

      BeginBenchmark("ecvcs_mainLoop");
      while(imageY < imageHeight) {
        //BeginBenchmark("ecvcs_filterRows");
        ecvcs_filterRows(integralImage, scaleImage_numPyramidLevels, imageY, imageWidth, &filteredRows[0]);
        //EndBenchmark("ecvcs_filterRows");

        //BeginBenchmark("ecvcs_computeBinaryImage");
        if(scaleImage_numPyramidLevels != 3) {
          ecvcs_computeBinaryImage(image, filteredRows, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, imageY, imageWidth, pBinaryImageRow);
        } else {
          ecvcs_computeBinaryImage_numPyramids3(image, filteredRows, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, imageY, imageWidth, pBinaryImageRow);
        }
        //EndBenchmark("ecvcs_computeBinaryImage");

        // Extract the next line of connected components
        //BeginBenchmark("ecvcs_extractNextRowOfComponents");
        if((lastResult = components.Extract2dComponents_PerRow_NextRow(pBinaryImageRow, imageWidth, imageY, component1d_minComponentWidth, component1d_maxSkipDistance)) != RESULT_OK)
          return lastResult;
        //EndBenchmark("ecvcs_extractNextRowOfComponents");

        //BeginBenchmark("ecvcs_scrollIntegralImage");

        imageY++;

        // If we've reached the bottom of this integral image, scroll it up
        if(integralImage.get_maxRow(maxFilterHalfWidth) < imageY) {
          if((lastResult = integralImage.ScrollDown(image, numRowsToScroll, fastScratch)) != RESULT_OK)
            return lastResult;
        }

        //EndBenchmark("ecvcs_scrollIntegralImage");
      } // while(imageY < size(image,1))

      EndBenchmark("ecvcs_mainLoop");

      BeginBenchmark("ecvcs_finalize");
      if((lastResult = components.Extract2dComponents_PerRow_Finalize()) != RESULT_OK)
        return lastResult;
      EndBenchmark("ecvcs_finalize");

      return RESULT_OK;
    } // ExtractComponentsViaCharacteristicScale
  } // namespace Embedded
} // namespace Anki
