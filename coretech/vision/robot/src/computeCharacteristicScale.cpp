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

    staticInline void ecvcs_filterRows(const ScrollingIntegralImage_u8_s32 &integralImage, const s32 scaleImage_numPyramidLevels, const s32 imageY, const s32 imageWidth, Array<s32> * restrict filteredRows);
    staticInline void ecvcs_computeBinaryImage(const Array<u8> &image, const Array<s32> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow);

    staticInline void ecvcs_filterRows(const ScrollingIntegralImage_u8_s32 &integralImage, const s32 scaleImage_numPyramidLevels, const s32 imageY, const s32 imageWidth, Array<s32> * restrict filteredRows)
    {
      // To normalize a sum of 1 / ((2*n+1)^2), we approximate a division as a mulitiply and shift.
      // These multiply coefficients were computed in matlab by typing:
      // " for i=1:5 disp(sprintf('i:%d',i')); computeClosestMultiplicationShift((2*(2^i)+1)^2, 31, 8); disp(sprintf('\n')); end; "
      const s32 normalizationMultiply[5] = {41, 101, 227, 241, 31}; // For halfWidths in [1,5]
      const s32 normalizationBitShifts[5] = {10, 13, 16, 18, 17};

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
    } // staticInline ecvcs_filterRows()

    staticInline void ecvcs_computeBinaryImage(const Array<u8> &image, const Array<s32> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow)
    {
      const s32 thresholdMultiplier_numFractionalBits = 16;

      const u8 * restrict pImage = image[imageY];

      if(scaleImage_numPyramidLevels == 3) {
        const s32 * restrict pFilteredRows0 = filteredRows[0][0];
        const s32 * restrict pFilteredRows1 = filteredRows[1][0];
        const s32 * restrict pFilteredRows2 = filteredRows[2][0];
        const s32 * restrict pFilteredRows3 = filteredRows[3][0];

        for(s32 x=0; x<imageWidth; x++) {
          s32 scaleValue;

          //for(s32 pyramidLevel=0; pyramidLevel<3; pyramidLevel++) {
          const s32 dog0 = ABS(pFilteredRows1[x] - pFilteredRows0[x]);
          const s32 dog1 = ABS(pFilteredRows2[x] - pFilteredRows1[x]);
          const s32 dog2 = ABS(pFilteredRows3[x] - pFilteredRows2[x]);

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
      } else { // if scaleImage_numPyramidLevels != 3
        const s32 * restrict pFilteredRows[5];
        for(s32 pyramidLevel=0; pyramidLevel<=scaleImage_numPyramidLevels; pyramidLevel++) {
          pFilteredRows[pyramidLevel] = filteredRows[pyramidLevel][0];
        }

        for(s32 x=0; x<imageWidth; x++) {
          s32 scaleValue = -1;
          s32 dogMax = s32_MIN;
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
      } // if(scaleImage_numPyramidLevels == 3) ... else
    } // staticInline void ecvcs_computeBinaryImage()

    Result ExtractComponentsViaCharacteristicScale(
      const Array<u8> &image,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack scratch)
    {
      BeginBenchmark("ecvcs_init");

      // Integral image constants
      const s32 numBorderPixels = 17; // For qvga, 2^4 + 1 = 17
      const s32 integralImageHeight = 64; // 96*(640+33*2)*4 = 271104, though with padding, it is 96*720*4 = 276480
      const s32 numRowsToScroll = integralImageHeight - 2*numBorderPixels;

      const s32 maxFilterHalfWidth = 1 << (scaleImage_numPyramidLevels+1);

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      Result lastResult;

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale", "image is not valid");

      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale", "components is not valid");

      AnkiConditionalErrorAndReturnValue(scaleImage_numPyramidLevels <= 4 && scaleImage_numPyramidLevels > 0,
        RESULT_FAIL_INVALID_PARAMETERS, "ExtractComponentsViaCharacteristicScale", "scaleImage_numPyramidLevels must be less than %d", 4+1);

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
        RESULT_FAIL_OUT_OF_MEMORY, "ExtractComponentsViaCharacteristicScale", "binaryImageRow is not valid");

      if((lastResult = components.Extract2dComponents_PerRow_Initialize(scratch)) != RESULT_OK)
        return lastResult;

      s32 imageY = 0;

      EndBenchmark("ecvcs_init");

      BeginBenchmark("ecvcs_mainLoop");
      while(imageY < imageHeight) {
        BeginBenchmark("ecvcs_filterRows");
        ecvcs_filterRows(integralImage, scaleImage_numPyramidLevels, imageY, imageWidth, &filteredRows[0]);
        EndBenchmark("ecvcs_filterRows");

        BeginBenchmark("ecvcs_computeBinaryImage");
        ecvcs_computeBinaryImage(image, filteredRows, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, imageY, imageWidth, pBinaryImageRow);
        EndBenchmark("ecvcs_computeBinaryImage");

        // Extract the next line of connected components
        BeginBenchmark("ecvcs_extractNextRowOfComponents");
        if((lastResult = components.Extract2dComponents_PerRow_NextRow(pBinaryImageRow, imageWidth, imageY, component1d_minComponentWidth, component1d_maxSkipDistance)) != RESULT_OK)
          return lastResult;
        EndBenchmark("ecvcs_extractNextRowOfComponents");

        BeginBenchmark("ecvcs_scrollIntegralImage");

        imageY++;

        // If we've reached the bottom of this integral image, scroll it up
        if(integralImage.get_maxRow(maxFilterHalfWidth) < imageY) {
          if((lastResult = integralImage.ScrollDown(image, numRowsToScroll, scratch)) != RESULT_OK)
            return lastResult;
        }

        EndBenchmark("ecvcs_scrollIntegralImage");
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
