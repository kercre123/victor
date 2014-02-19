/**
File: computeCharacteristicScale.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/utilities.h"
#include "anki/common/robot/benchmarking_c.h"
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

    Result ExtractComponentsViaCharacteristicScale(
      const Array<u8> &image,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack scratch)
    {
      BeginBenchmark("ccsiabaec_init");
      const s32 thresholdMultiplier_numFractionalBits = 16;

      // Integral image constants
      const s32 numBorderPixels = 17; // For qvga, 2^4 + 1 = 17
      const s32 integralImageHeight = 64; // 96*(640+33*2)*4 = 271104, though with padding, it is 96*720*4 = 276480
      const s32 numRowsToScroll = integralImageHeight - 2*numBorderPixels;

      // To normalize a sum of 1 / ((2*n+1)^2), we approximate a division as a mulitiply and shift.
      // These multiply coefficients were computed in matlab by typing:
      // " for i=1:5 disp(sprintf('i:%d',i')); computeClosestMultiplicationShift((2*(2^i)+1)^2, 31, 8); disp(sprintf('\n')); end; "
      const s32 normalizationMultiply[5] = {41, 101, 227, 241, 31}; // For halfWidths in [1,5]
      const s32 normalizationBitShifts[5] = {10, 13, 16, 18, 17};

      const s32 maxFilterHalfWidth = 1 << (scaleImage_numPyramidLevels+1);

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      Result lastResult;

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeCharacteristicScaleImageAndBinarize", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeCharacteristicScaleImageAndBinarize", "image is not valid");

      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeCharacteristicScaleImageAndBinarize", "components is not valid");

      AnkiConditionalErrorAndReturnValue(scaleImage_numPyramidLevels <= 4 && scaleImage_numPyramidLevels > 0,
        RESULT_FAIL_INVALID_PARAMETERS, "ComputeCharacteristicScaleImageAndBinarize", "scaleImage_numPyramidLevels must be less than %d", 4+1);

      // Initialize the first integralImageHeight rows of the integralImage
      ScrollingIntegralImage_u8_s32 integralImage(integralImageHeight, imageWidth, numBorderPixels, scratch);
      if((lastResult = integralImage.ScrollDown(image, integralImageHeight, scratch)) != RESULT_OK)
        return lastResult;

      // Prepare the memory for the filtered rows for each level of the pyramid
      Array<s32> filteredRows[5];
      for(s32 i=0; i<=scaleImage_numPyramidLevels; i++) {
        filteredRows[i] = Array<s32>(1, imageWidth, scratch);
      }

      Array<u8> binaryImageRow(1, imageWidth, scratch);
      u8 * restrict pBinaryImageRow = binaryImageRow[0];

      AnkiConditionalErrorAndReturnValue(binaryImageRow.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "ComputeCharacteristicScaleImageAndBinarize", "binaryImageRow is not valid");

      if((lastResult = components.Extract2dComponents_PerRow_Initialize(scratch)) != RESULT_OK)
        return lastResult;

      s32 imageY = 0;

      EndBenchmark("ccsiabaec_init");

      BeginBenchmark("ccsiabaec_mainLoop");
      while(imageY < imageHeight) {
        BeginBenchmark("ccsiabaec_filterRows");

        // For the given row of the image, compute the blurred version for each level of the pyramid
        for(s32 pyramidLevel=0; pyramidLevel<=scaleImage_numPyramidLevels; pyramidLevel++) {
          const s32 filterHalfWidth = 1 << (pyramidLevel+1); // filterHalfWidth = 2^pyramidLevel;
          const Rectangle<s16> filter(-filterHalfWidth, filterHalfWidth, -filterHalfWidth, filterHalfWidth);

          // Compute the sums using the integral image
          integralImage.FilterRow(filter, imageY, filteredRows[pyramidLevel]);

          // Normalize the sums
          s32 * restrict pFilteredRow = filteredRows[pyramidLevel][0];
          const s32 multiply = normalizationMultiply[pyramidLevel];
          const s32 shift = normalizationBitShifts[pyramidLevel];
          for(s32 x=0; x<imageWidth; x++) {
            pFilteredRow[x] = (pFilteredRow[x] * multiply) >> shift;
          }
        } // for(s32 pyramidLevel=0; pyramidLevel<=numLevels; pyramidLevel++)

        const s32 * restrict pFilteredRows[5];
        for(s32 pyramidLevel=0; pyramidLevel<=scaleImage_numPyramidLevels; pyramidLevel++) {
          pFilteredRows[pyramidLevel] = filteredRows[pyramidLevel][0];
        }

        EndBenchmark("ccsiabaec_filterRows");

        BeginBenchmark("ccsiabaec_computeBinaryImage");

        const u8 * restrict pImage = image[imageY];

        for(s32 x=0; x<imageWidth; x++) {
          s32 dogMax = s32_MIN;
          s32 scaleValue = -1;
          for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; pyramidLevel++) {
            const s32 dog = ABS(pFilteredRows[pyramidLevel+1][x] - pFilteredRows[pyramidLevel][x]);

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

        EndBenchmark("ccsiabaec_computeBinaryImage");

        BeginBenchmark("ccsiabaec_extractNextRowOfComponents");

        // Extract the next line of connected components
        if((lastResult = components.Extract2dComponents_PerRow_NextRow(pBinaryImageRow, imageWidth, imageY, component1d_minComponentWidth, component1d_maxSkipDistance)) != RESULT_OK)
          return lastResult;

        EndBenchmark("ccsiabaec_extractNextRowOfComponents");

        BeginBenchmark("ccsiabaec_scrollIntegralImage");

        imageY++;

        //% If we've reached the bottom of this integral image, scroll it up
        if(integralImage.get_maxRow(maxFilterHalfWidth) < imageY) {
          if((lastResult = integralImage.ScrollDown(image, numRowsToScroll, scratch)) != RESULT_OK)
            return lastResult;
        }

        EndBenchmark("ccsiabaec_scrollIntegralImage");
      } // while(imageY < size(image,1))

      EndBenchmark("ccsiabaec_mainLoop");

      BeginBenchmark("ccsiabaec_finalize");
      if((lastResult = components.Extract2dComponents_PerRow_Finalize()) != RESULT_OK)
        return lastResult;
      EndBenchmark("ccsiabaec_finalize");

      return RESULT_OK;
    } // ExtractComponentsViaCharacteristicScale
  } // namespace Embedded
} // namespace Anki
