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
#include "anki/common/robot/hostIntrinsics_m4.h"

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/integralImage.h"
#include "anki/vision/robot/imageProcessing.h"

#define USE_ARM_ACCELERATION

#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif

static const s32 MAX_FILTER_HALF_WIDTH = 64;

// To normalize a sum of 1 / ((2*n+1)^2), we approximate a division as a mulitiply and shift.
// These multiply coefficients were computed in matlab by typing:
// n=65; bestShifts = zeros(n,1); bestMultipliers = zeros(n,1); for i=0:(n-1) [bestShifts(i+1), bestMultipliers(i+1)] = computeClosestMultiplicationShift((2*i+1)^2, 31, 8); end; toArray(bestMultipliers'); toArray(bestShifts');
static const s32 normalizationMultiply[MAX_FILTER_HALF_WIDTH+1] = {1, 57, 41, 167, 101, 135, 97, 73, 227, 91, 149, 31, 105, 45, 39, 17, 241, 107, 191, 43, 39, 71, 129, 237, 109, 101, 187, 173, 161, 151, 141, 33, 31, 117, 55, 13, 197, 93, 177, 21, 5, 19, 145, 139, 33, 253, 121, 29, 223, 107, 103, 99, 95, 183, 177, 85, 41, 159, 153, 37, 143, 139, 67, 65, 63};
static const s32 normalizationBitShifts[MAX_FILTER_HALF_WIDTH+1] = {0, 9, 10, 13, 13, 14, 14, 14, 16, 15, 16, 14, 16, 15, 15, 14, 18, 17, 18, 16, 16, 17, 18, 19, 18, 18, 19, 19, 19, 19, 19, 17, 17, 19, 18, 16, 20, 19, 20, 17, 15, 17, 20, 20, 18, 21, 20, 18, 21, 20, 20, 20, 20, 21, 21, 20, 19, 21, 21, 19, 21, 21, 20, 20, 20};

namespace Anki
{
  namespace Embedded
  {
    // These are not inlined, to make it easier to hand-optimize them. Inlining them will probably only slightly increase speed.
    NO_INLINE void ecvcs_filterRows(const ScrollingIntegralImage_u8_s32 &integralImage, const FixedLengthList<s32> &filterHalfWidths, const s32 imageY, FixedLengthList<Array<u8> > &filteredRows);

    /*NO_INLINE void ecvcs_computeBinaryImage_numPyramids3(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow);
    NO_INLINE void ecvcs_computeBinaryImage_numPyramids3_thresholdMultiplier1(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow);*/
    NO_INLINE void ecvcs_computeBinaryImage(const Array<u8> &image, FixedLengthList<Array<u8> > &filteredRows, const s32 scaleImage_thresholdMultiplier, const s32 imageY, u8 * restrict pBinaryImageRow);

    NO_INLINE void ecvcs_filterRows(const ScrollingIntegralImage_u8_s32 &integralImage, const FixedLengthList<s32> &filterHalfWidths, const s32 imageY, FixedLengthList<Array<u8> > &filteredRows)
    {
      const s32 numFilterHalfWidths = filterHalfWidths.get_size();

      AnkiAssert(numFilterHalfWidths == filteredRows.get_size());

      const s32 * restrict pFilterHalfWidths = filterHalfWidths.Pointer(0);
      Array<u8> * restrict pFilteredRows = filteredRows.Pointer(0);

      // For the given row of the image, compute the blurred version for each level of the pyramid
      for(s32 iHalfWidth=0; iHalfWidth<numFilterHalfWidths; iHalfWidth++) {
        //BeginBenchmark("ecvcs_filterRows_init");
        const s32 filterHalfWidth = pFilterHalfWidths[iHalfWidth];

        const Rectangle<s16> filter(-filterHalfWidth, filterHalfWidth, -filterHalfWidth, filterHalfWidth);

        const s32 multiply = normalizationMultiply[filterHalfWidth];
        const s32 shift = normalizationBitShifts[filterHalfWidth];

        //EndBenchmark("ecvcs_filterRows_init");

        //BeginBenchmark("ecvcs_filterRows_filter&normalize");

        // Compute the sums using the integral image
        integralImage.FilterRow(filter, imageY, multiply, shift, pFilteredRows[iHalfWidth]);

        //EndBenchmark("ecvcs_filterRows_filter&normalize");
      } // for(s32 pyramidLevel=0; pyramidLevel<=numLevels; pyramidLevel++)
    } // staticInline ecvcs_filterRows()

    /*
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
    */

    /*
    NO_INLINE void ecvcs_computeBinaryImage_numPyramids3_thresholdMultiplier1(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow)
    {
    const u32 * restrict pImage = reinterpret_cast<const u32*>(image[imageY]);

    const u32 * restrict pFilteredRows0 = reinterpret_cast<const u32*>(filteredRows[0][0]);
    const u32 * restrict pFilteredRows1 = reinterpret_cast<const u32*>(filteredRows[1][0]);
    const u32 * restrict pFilteredRows2 = reinterpret_cast<const u32*>(filteredRows[2][0]);
    const u32 * restrict pFilteredRows3 = reinterpret_cast<const u32*>(filteredRows[3][0]);

    u32 * restrict pBinaryImageRowU32 = reinterpret_cast<u32*>(pBinaryImageRow);

    const s32 imageWidth4 = imageWidth / 4;

    #if defined(USE_ARM_ACCELERATION)
    const u32 twoFiftyFourX4 = 0xFEFEFEFEU;
    #endif

    for(s32 x=0; x<imageWidth4; x++) {
    const u32 filteredRows0 = pFilteredRows0[x];
    const u32 filteredRows1 = pFilteredRows1[x];
    const u32 filteredRows2 = pFilteredRows2[x];
    const u32 filteredRows3 = pFilteredRows3[x];

    #if !defined(USE_ARM_ACCELERATION)
    const s32 dog0_row0 = ABS(static_cast<s32>(filteredRows1 & 0xFF) - static_cast<s32>(filteredRows0 & 0xFF));
    const s32 dog1_row0 = ABS(static_cast<s32>(filteredRows2 & 0xFF) - static_cast<s32>(filteredRows1 & 0xFF));
    const s32 dog2_row0 = ABS(static_cast<s32>(filteredRows3 & 0xFF) - static_cast<s32>(filteredRows2 & 0xFF));

    const s32 dog0_row1 = ABS(static_cast<s32>((filteredRows1 & 0xFF00) >> 8) - static_cast<s32>((filteredRows0 & 0xFF00) >> 8));
    const s32 dog1_row1 = ABS(static_cast<s32>((filteredRows2 & 0xFF00) >> 8) - static_cast<s32>((filteredRows1 & 0xFF00) >> 8));
    const s32 dog2_row1 = ABS(static_cast<s32>((filteredRows3 & 0xFF00) >> 8) - static_cast<s32>((filteredRows2 & 0xFF00) >> 8));

    const s32 dog0_row2 = ABS(static_cast<s32>((filteredRows1 & 0xFF0000) >> 16) - static_cast<s32>((filteredRows0 & 0xFF0000) >> 16));
    const s32 dog1_row2 = ABS(static_cast<s32>((filteredRows2 & 0xFF0000) >> 16) - static_cast<s32>((filteredRows1 & 0xFF0000) >> 16));
    const s32 dog2_row2 = ABS(static_cast<s32>((filteredRows3 & 0xFF0000) >> 16) - static_cast<s32>((filteredRows2 & 0xFF0000) >> 16));

    const s32 dog0_row3 = ABS(static_cast<s32>((filteredRows1 & 0xFF000000) >> 24) - static_cast<s32>((filteredRows0 & 0xFF000000) >> 24));
    const s32 dog1_row3 = ABS(static_cast<s32>((filteredRows2 & 0xFF000000) >> 24) - static_cast<s32>((filteredRows1 & 0xFF000000) >> 24));
    const s32 dog2_row3 = ABS(static_cast<s32>((filteredRows3 & 0xFF000000) >> 24) - static_cast<s32>((filteredRows2 & 0xFF000000) >> 24));

    u32 scaleValue_row0;
    u32 scaleValue_row1;
    u32 scaleValue_row2;
    u32 scaleValue_row3;

    if(dog0_row0 > dog1_row0) {
    if(dog0_row0 > dog2_row0) { scaleValue_row0 = pFilteredRows1[x] & 0xFF; }
    else { scaleValue_row0 = pFilteredRows3[x] & 0xFF; }
    } else if(dog1_row0 > dog2_row0) {
    scaleValue_row0 = pFilteredRows2[x] & 0xFF;
    } else {
    scaleValue_row0 = pFilteredRows3[x] & 0xFF;
    }

    if(dog0_row1 > dog1_row1) {
    if(dog0_row1 > dog2_row1) { scaleValue_row1 = (pFilteredRows1[x] & 0xFF00) >> 8; }
    else { scaleValue_row1 = (pFilteredRows3[x] & 0xFF00) >> 8; }
    } else if(dog1_row1 > dog2_row1) {
    scaleValue_row1 = (pFilteredRows2[x] & 0xFF00) >> 8;
    } else {
    scaleValue_row1 = (pFilteredRows3[x] & 0xFF00) >> 8;
    }

    if(dog0_row2 > dog1_row2) {
    if(dog0_row2 > dog2_row2) { scaleValue_row2 = (pFilteredRows1[x] & 0xFF0000) >> 16; }
    else { scaleValue_row2 = (pFilteredRows3[x] & 0xFF0000) >> 16; }
    } else if(dog1_row2 > dog2_row2) {
    scaleValue_row2 = (pFilteredRows2[x] & 0xFF0000) >> 16;
    } else {
    scaleValue_row2 = (pFilteredRows3[x] & 0xFF0000) >> 16;
    }

    if(dog0_row3 > dog1_row3) {
    if(dog0_row3 > dog2_row3) { scaleValue_row3 = (pFilteredRows1[x] & 0xFF000000) >> 24; }
    else { scaleValue_row3 = (pFilteredRows3[x] & 0xFF000000) >> 24; }
    } else if(dog1_row3 > dog2_row3) {
    scaleValue_row3 = (pFilteredRows2[x] & 0xFF000000) >> 24;
    } else {
    scaleValue_row3 = (pFilteredRows3[x] & 0xFF000000) >> 24;
    }

    const u32 thresholdValue_row0 = scaleValue_row0;
    const u32 thresholdValue_row1 = scaleValue_row1;
    const u32 thresholdValue_row2 = scaleValue_row2;
    const u32 thresholdValue_row3 = scaleValue_row3;

    const u32 curPixel = pImage[x];

    u32 binaryRow = 0;
    if((curPixel & 0xFF)               < thresholdValue_row0) { binaryRow |= 1; }
    if(((curPixel & 0xFF00) >> 8)      < thresholdValue_row1) { binaryRow |= (1 << 8); }
    if(((curPixel & 0xFF0000) >> 16)   < thresholdValue_row2) { binaryRow |= (1 << 16); }
    if(((curPixel & 0xFF000000) >> 24) < thresholdValue_row3) { binaryRow |= (1 << 24); }
    #else // if !defined(USE_ARM_ACCELERATION)
    // M4 intrinsic version of 4-way simd calculation

    //
    // Compute the Difference of Gaussians (DoG)
    //

    // dog0 is 4-way absolute value of filteredRows0 and filteredRows1
    const u32 uqsub10 = __UQSUB8(filteredRows1, filteredRows0);
    const u32 uqsub01 = __UQSUB8(filteredRows0, filteredRows1);
    const u32 dog0 = uqsub10 | uqsub01;

    const u32 uqsub21 = __UQSUB8(filteredRows2, filteredRows1);
    const u32 uqsub12 = __UQSUB8(filteredRows1, filteredRows2);
    const u32 dog1 = uqsub21 | uqsub12;

    const u32 uqsub32 = __UQSUB8(filteredRows3, filteredRows2);
    const u32 uqsub23 = __UQSUB8(filteredRows2, filteredRows3);
    const u32 dog2 = uqsub32 | uqsub23;

    //
    // Compute the filteredRows corresponding to the maximum Difference of Gaussian
    //

    // dogMax012 is 4-way max of dog0, dog1, and dog2
    __USUB8(dog0, dog1); // The answer is irrelevant, we just need to set the GE bits
    const u32 dogMax01 = __SEL(dog0, dog1);

    __USUB8(dogMax01, dog2); // The answer is irrelevant, we just need to set the GE bits
    const u32 dogMax012 = __SEL(dogMax01, dog2);

    // For the maximum dog, put the corresponding filtered row into thresholdValue
    u32 thresholdValue = filteredRows1;

    __USUB8(dog1, dogMax012); // The answer is irrelevant, we just need to set the GE bits
    thresholdValue = __SEL(filteredRows2, thresholdValue);

    __USUB8(dog2, dogMax012); // The answer is irrelevant, we just need to set the GE bits
    thresholdValue = __SEL(filteredRows3, thresholdValue);

    //
    // Binarize the output
    //

    const u32 curPixel = pImage[x];

    const u32 compareWithThreshold = __UQSUB8(thresholdValue, curPixel);
    const u32 compareWithThresholdAndSaturate = __UQADD8(compareWithThreshold, twoFiftyFourX4);
    const u32 binaryRow = __UQSUB8(compareWithThresholdAndSaturate, twoFiftyFourX4);
    #endif // if !defined(USE_ARM_ACCELERATION) ... #else

    pBinaryImageRowU32[x] = binaryRow;
    } // for(s32 x=0; x<imageWidth4; x++)
    } // staticInline void ecvcs_computeBinaryImage()
    */

    NO_INLINE void ecvcs_computeBinaryImage(const Array<u8> &image, FixedLengthList<Array<u8> > &filteredRows, const s32 scaleImage_thresholdMultiplier, const s32 imageY, u8 * restrict pBinaryImageRow)
    {
      const s32 thresholdMultiplier_numFractionalBits = 16;

      const u8 * restrict pImage = image[imageY];

      const s32 imageWidth = image.get_size(1);

      const s32 numFilteredRows = filteredRows.get_size();

      AnkiAssert(filteredRows.get_size() <= MAX_FILTER_HALF_WIDTH);

      const u8 * restrict pFilteredRows[MAX_FILTER_HALF_WIDTH+1];
      for(s32 i=0; i<numFilteredRows; i++) {
        pFilteredRows[i] = filteredRows[i][0];
      }

      for(s32 x=0; x<imageWidth; x++) {
        s32 scaleValue = -1;
        s32 dogMax = s32_MIN;
        for(s32 iHalfWidth=0; iHalfWidth<(numFilteredRows-1); iHalfWidth++) {
          const s32 dog = ABS(static_cast<s32>(pFilteredRows[iHalfWidth+1][x]) - static_cast<s32>(pFilteredRows[iHalfWidth][x]));

          if(dog > dogMax) {
            dogMax = dog;
            scaleValue = pFilteredRows[iHalfWidth+1][x];
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
      const FixedLengthList<s32> &filterHalfWidths, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack fastScratch,
      MemoryStack slowerScratch,
      MemoryStack slowestScratch)
    {
      BeginBenchmark("ecvcs_init");

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      const s32 numFilterHalfWidths = filterHalfWidths.get_size();

      AnkiConditionalErrorAndReturnValue(AreValid(fastScratch, slowerScratch, slowestScratch),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(NotAliased(fastScratch, slowerScratch, slowestScratch),
        RESULT_FAIL_ALIASED_MEMORY, "ExtractComponentsViaCharacteristicScale", "fast and slow scratch buffers cannot be the same object");

      AnkiConditionalErrorAndReturnValue(AreValid(image, filterHalfWidths, components),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(numFilterHalfWidths > 0 && numFilterHalfWidths <= MAX_FILTER_HALF_WIDTH,
        RESULT_FAIL_INVALID_PARAMETER, "ExtractComponentsViaCharacteristicScale", "invalid numFilterHalfWidths");

      Result lastResult;

      const s32 * restrict pFilterHalfWidths = filterHalfWidths.Pointer(0);

      // Integral image constants
      s32 numBorderPixels = -1;
      s32 integralImageHeight;

      for(s32 i=0; i<numFilterHalfWidths; i++) {
        AnkiAssert(pFilterHalfWidths[i] >= 0);
        AnkiAssert(pFilterHalfWidths[i] <= MAX_FILTER_HALF_WIDTH);
        numBorderPixels = MAX(numBorderPixels, pFilterHalfWidths[i] + 1);
      }

      // TODO: choose based on amount of free memory
      if(imageWidth <= 320) {
        integralImageHeight = MAX(2*numBorderPixels + 16, 64); // 96*(640+33*2)*4 = 271104, though with padding, it is 96*720*4 = 276480
      } else {
        // Note: These numbers are liable to be too big to fit on the M4 efficiently
        const s32 scaleFactor = static_cast<s32>(ceilf(static_cast<f32>(imageWidth) / 320.0f));
        integralImageHeight = MAX(2*numBorderPixels + 16*scaleFactor, 64*scaleFactor);
      }

      const s32 numRowsToScroll = integralImageHeight - 2*numBorderPixels;
      
      AnkiAssert(numRowsToScroll > 1);

      // Initialize the first integralImageHeight rows of the integralImage
      ScrollingIntegralImage_u8_s32 integralImage(integralImageHeight, imageWidth, numBorderPixels, slowerScratch);
      if((lastResult = integralImage.ScrollDown(image, integralImageHeight, fastScratch)) != RESULT_OK)
        return lastResult;

      // Prepare the memory for the filtered rows for each level of the pyramid
      FixedLengthList<Array<u8> > filteredRows(numFilterHalfWidths, fastScratch, Flags::Buffer(false,false,true));

      AnkiConditionalErrorAndReturnValue(filteredRows.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "ExtractComponentsViaCharacteristicScale", "filteredRows is not valid");

      for(s32 i=0; i<numFilterHalfWidths; i++) {
        filteredRows[i] = Array<u8>(1, imageWidth, fastScratch);
        AnkiConditionalErrorAndReturnValue(filteredRows[i].IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "ExtractComponentsViaCharacteristicScale", "filteredRows is not valid");
      }

      Array<u8> binaryImageRow(1, imageWidth, fastScratch);

      AnkiConditionalErrorAndReturnValue(binaryImageRow.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "ExtractComponentsViaCharacteristicScale", "binaryImageRow is not valid");

      u8 * restrict pBinaryImageRow = binaryImageRow[0];

      if((lastResult = components.Extract2dComponents_PerRow_Initialize(fastScratch, slowerScratch, slowestScratch)) != RESULT_OK)
        return lastResult;

      s32 imageY = 0;

      EndBenchmark("ecvcs_init");

      BeginBenchmark("ecvcs_mainLoop");
      while(imageY < imageHeight) {
        //BeginBenchmark("ecvcs_filterRows");
        ecvcs_filterRows(integralImage, filterHalfWidths, imageY, filteredRows);
        //EndBenchmark("ecvcs_filterRows");

        //BeginBenchmark("ecvcs_computeBinaryImage");
        /*
        if(scaleImage_numPyramidLevels != 3) {
        ecvcs_computeBinaryImage(image, filteredRows, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, imageY, imageWidth, pBinaryImageRow);
        } else {
        if(scaleImage_thresholdMultiplier == 65536) {
        ecvcs_computeBinaryImage_numPyramids3_thresholdMultiplier1(image, filteredRows, scaleImage_numPyramidLevels, imageY, imageWidth, pBinaryImageRow);
        } else {
        ecvcs_computeBinaryImage_numPyramids3(image, filteredRows, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, imageY, imageWidth, pBinaryImageRow);
        }
        }
        */

        ecvcs_computeBinaryImage(image, filteredRows, scaleImage_thresholdMultiplier, imageY, pBinaryImageRow);

        //EndBenchmark("ecvcs_computeBinaryImage");

        // Extract the next line of connected components
        //BeginBenchmark("ecvcs_extractNextRowOfComponents");
        if((lastResult = components.Extract2dComponents_PerRow_NextRow(pBinaryImageRow, imageWidth, imageY, component1d_minComponentWidth, component1d_maxSkipDistance)) != RESULT_OK)
          return lastResult;
        //EndBenchmark("ecvcs_extractNextRowOfComponents");

        //BeginBenchmark("ecvcs_scrollIntegralImage");

        imageY++;

        // If we've reached the bottom of this integral image, scroll it up
        if(integralImage.get_maxRow(numBorderPixels-1) < imageY) {
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
