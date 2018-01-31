/**
File: computeCharacteristicScale.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/utilities.h"
#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/interpolate.h"
#include "coretech/common/robot/hostIntrinsics_m4.h"

#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/integralImage.h"
#include "coretech/vision/robot/imageProcessing.h"

#define STORE_BINARY_IMAGE 0
#if STORE_BINARY_IMAGE
#warning Storing binary image
bool g_saveBinaryImage = true;
bool g_saveBinaryInitialized = false;
Anki::Embedded::Array<u8> g_binaryImage;
const static s32 binaryImageBufferLength = 1920*1080 + 1024;
static char binaryImageBuffer[binaryImageBufferLength];
#else
bool g_saveBinaryImage = false;
Anki::Embedded::Array<u8> g_binaryImage;
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

    //NO_INLINE void ecvcs_computeBinaryImage_numFilters5(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow);
    //NO_INLINE void ecvcs_computeBinaryImage_numPyramids3_thresholdMultiplier1(const Array<u8> &image, const Array<u8> * restrict filteredRows, const s32 scaleImage_numPyramidLevels, const s32 imageY, const s32 imageWidth, u8 * restrict pBinaryImageRow);

    NO_INLINE void ecvcs_computeBinaryImage(const Array<u8> &image,
                                            FixedLengthList<Array<u8> > &filteredRows,
                                            const s32 scaleImage_thresholdMultiplier,
                                            const s32 imageY,
                                            u8 * restrict pBinaryImageRow,
                                            const bool isDarkOnLight);
    
    NO_INLINE void ecvcs_computeBinaryImage_numFilters3(const Array<u8> &image,
                                                        FixedLengthList<Array<u8> > &filteredRows,
                                                        const s32 scaleImage_thresholdMultiplier,
                                                        const s32 imageY,
                                                        u8 * restrict pBinaryImageRow,
                                                        const bool isDarkOnLight);
    
    NO_INLINE void ecvcs_computeBinaryImage_numFilters5(const Array<u8> &image,
                                                        FixedLengthList<Array<u8> > &filteredRows,
                                                        const s32 scaleImage_thresholdMultiplier,
                                                        const s32 imageY,
                                                        u8 * restrict pBinaryImageRow,
                                                        const bool isDarkOnLight);
    
    NO_INLINE void ecvcs_computeBinaryImage_numFilters5_thresholdMultiplier1(const Array<u8> &image,
                                                                             FixedLengthList<Array<u8> > &filteredRows,
                                                                             const s32 scaleImage_thresholdMultiplier,
                                                                             const s32 imageY,
                                                                             u8 * restrict pBinaryImageRow,
                                                                             const bool isDarkOnLight);

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


    NO_INLINE void ecvcs_computeBinaryImage_numFilters3(const Array<u8> &image, FixedLengthList<Array<u8> > &filteredRows, 
                                                        const s32 scaleImage_thresholdMultiplier, 
                                                        const s32 imageY, u8 * restrict pBinaryImageRow, 
                                                        const bool isDarkOnLight)
    {
      AnkiAssert(filteredRows.get_size() == 3);
      
      static const s32 thresholdMultiplier_numFractionalBits = 16;
      
      const u8 * restrict pImage = image[imageY];
      
      const u8 * restrict pFilteredRows0 = filteredRows[0][0];
      const u8 * restrict pFilteredRows1 = filteredRows[1][0];
      const u8 * restrict pFilteredRows2 = filteredRows[2][0];
      
      const s32 imageWidth = image.get_size(1);
            
      AnkiAssert(filteredRows.get_size() <= MAX_FILTER_HALF_WIDTH);

#if ACCELERATION_TYPE == ACCELERATION_ARM_A7

      const int32x4_t kThreshMult = vdupq_n_s32(scaleImage_thresholdMultiplier);
      const int32x4_t kFracBits = vdupq_n_s32(-thresholdMultiplier_numFractionalBits); // Negative for right shift
      const uint8x16_t kZeros = vdupq_n_u8(!isDarkOnLight);
      const uint8x16_t kOnes = vdupq_n_u8(isDarkOnLight);

      const u32 kNumElementsProcessedPerLoop = 16;
      const u32 kNumIterations = imageWidth - (kNumElementsProcessedPerLoop - 1);

      s32 x = 0;
      for(; x < kNumIterations; x += kNumElementsProcessedPerLoop)
      {
        // Load FilteredRows
        uint8x16_t rows0 = vld1q_u8(pFilteredRows0);
        pFilteredRows0 += kNumElementsProcessedPerLoop;
        uint8x16_t rows1 = vld1q_u8(pFilteredRows1);
        pFilteredRows1 += kNumElementsProcessedPerLoop;
        uint8x16_t rows2 = vld1q_u8(pFilteredRows2);
        pFilteredRows2 += kNumElementsProcessedPerLoop;

        // Figure out which elements from the rows to use as the scale values
        uint8x16_t dog0 = vabdq_u8(rows1, rows0); // Absolute difference
        uint8x16_t dog1 = vabdq_u8(rows2, rows1);
        // Get all the elements in dog0 that are greater than dog1
        uint8x16_t dog0GTdog1 = vcgtq_u8(dog0, dog1);
        // If dog0 > dog1 select the value from rows1 otherwise use rows2
        uint8x16_t scaleValue = vbslq_u8(dog0GTdog1, rows1, rows2);

        // The following block converts scaleValue from a uint8x16 to 4 uint32x4s
        uint8x8_t scale8x8_1 = vget_low_u8(scaleValue);
        uint8x8_t scale8x8_2 = vget_high_u8(scaleValue);
        uint16x8_t scale16x8_1 = vmovl_u8(scale8x8_1);
        uint16x8_t scale16x8_2 = vmovl_u8(scale8x8_2);
        uint16x4_t scale16x4_1_1 = vget_low_u16(scale16x8_1);
        uint16x4_t scale16x4_1_2 = vget_high_u16(scale16x8_1);
        uint16x4_t scale16x4_2_1 = vget_low_u16(scale16x8_2);
        uint16x4_t scale16x4_2_2 = vget_high_u16(scale16x8_2);
        uint32x4_t scale32x4_1 = vmovl_u16(scale16x4_1_1);
        uint32x4_t scale32x4_2 = vmovl_u16(scale16x4_1_2);
        uint32x4_t scale32x4_3 = vmovl_u16(scale16x4_2_1);
        uint32x4_t scale32x4_4 = vmovl_u16(scale16x4_2_2);

        // Multiple scale by threshold multiplier
        int32x4_t thresholdValue_1 = vmulq_s32((int32x4_t)scale32x4_1, kThreshMult);
        int32x4_t thresholdValue_2 = vmulq_s32((int32x4_t)scale32x4_2, kThreshMult);
        int32x4_t thresholdValue_3 = vmulq_s32((int32x4_t)scale32x4_3, kThreshMult);
        int32x4_t thresholdValue_4 = vmulq_s32((int32x4_t)scale32x4_4, kThreshMult);
        
        // Shift by numFracBits
        thresholdValue_1 = vshlq_s32(thresholdValue_1, kFracBits);
        thresholdValue_2 = vshlq_s32(thresholdValue_2, kFracBits);
        thresholdValue_3 = vshlq_s32(thresholdValue_3, kFracBits);
        thresholdValue_4 = vshlq_s32(thresholdValue_4, kFracBits);

        // Load image data and convert from uint8x16 to 4 uint32x4s
        uint8x16_t img = vld1q_u8(pImage);
        pImage += kNumElementsProcessedPerLoop;
        uint8x8_t img8x8_1 = vget_low_u8(img);
        uint8x8_t img8x8_2 = vget_high_u8(img);
        uint16x8_t img16x8_1 = vmovl_u8(img8x8_1);
        uint16x8_t img16x8_2 = vmovl_u8(img8x8_2);
        uint16x4_t img16x4_1_1 = vget_low_u16(img16x8_1);
        uint16x4_t img16x4_1_2 = vget_high_u16(img16x8_1);
        uint16x4_t img16x4_2_1 = vget_low_u16(img16x8_2);
        uint16x4_t img16x4_2_2 = vget_high_u16(img16x8_2);
        uint32x4_t img32x4_1 = vmovl_u16(img16x4_1_1);
        uint32x4_t img32x4_2 = vmovl_u16(img16x4_1_2);
        uint32x4_t img32x4_3 = vmovl_u16(img16x4_2_1);
        uint32x4_t img32x4_4 = vmovl_u16(img16x4_2_2);

        // Compare image value to threshold, if it is less than a 1 will be written otherwise 0
        uint32x4_t lessThanThresh_1 = vcltq_s32((int32x4_t)img32x4_1, thresholdValue_1);
        uint32x4_t lessThanThresh_2 = vcltq_s32((int32x4_t)img32x4_2, thresholdValue_2);
        uint32x4_t lessThanThresh_3 = vcltq_s32((int32x4_t)img32x4_3, thresholdValue_3);
        uint32x4_t lessThanThresh_4 = vcltq_s32((int32x4_t)img32x4_4, thresholdValue_4);
        
        // Covert 4 uint32x4s to one uint8x16
        uint16x4_t lt16x4_1 = vmovn_u32(lessThanThresh_1);
        uint16x4_t lt16x4_2 = vmovn_u32(lessThanThresh_2);
        uint16x4_t lt16x4_3 = vmovn_u32(lessThanThresh_3);
        uint16x4_t lt16x4_4 = vmovn_u32(lessThanThresh_4);
        uint16x8_t lt16x8_1 = vcombine_u16(lt16x4_1, lt16x4_2);
        uint16x8_t lt16x8_2 = vcombine_u16(lt16x4_3, lt16x4_4);
        uint8x8_t  lt8x8_1 = vmovn_u16(lt16x8_1);
        uint8x8_t  lt8x8_2 = vmovn_u16(lt16x8_2);
        uint8x16_t lessThanThresh = vcombine_u8(lt8x8_1, lt8x8_2);

        // Use lessThanThresh to select either 1 or 0 to be written as output to the binary image
        uint8x16_t output = vbslq_u8(lessThanThresh, kOnes, kZeros);
        vst1q_u8(pBinaryImageRow, output);
        pBinaryImageRow += kNumElementsProcessedPerLoop;
      }

      for(; x < imageWidth; x++)
      {
        const s32 dog0 = ABS(static_cast<s32>(*pFilteredRows1) - static_cast<s32>(*pFilteredRows0));
        const s32 dog1 = ABS(static_cast<s32>(*pFilteredRows2) - static_cast<s32>(*pFilteredRows1));
        
        s32 scaleValue;
        
        if(dog0 > dog1)
        {
          scaleValue = *pFilteredRows1;
        }
        else
        {
          scaleValue = *pFilteredRows2;
        }
                
        const s32 thresholdValue = (scaleValue*scaleImage_thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;
        if(*pImage < thresholdValue) 
        {
          *pBinaryImageRow = 1;
        } 
        else
        {
          *pBinaryImageRow = 0;
        }

        pFilteredRows0++;
        pFilteredRows1++;
        pFilteredRows2++;
        pBinaryImageRow++;
      }

#else
      
      for(s32 x=0; x<imageWidth; x++) 
      {
        //for(s32 iHalfWidth=0; iHalfWidth<(numFilteredRows-1); iHalfWidth++) {
        const s32 dog0 = ABS(static_cast<s32>(pFilteredRows1[x]) - static_cast<s32>(pFilteredRows0[x]));
        const s32 dog1 = ABS(static_cast<s32>(pFilteredRows2[x]) - static_cast<s32>(pFilteredRows1[x]));
        
        s32 scaleValue;
        
        if(dog0 > dog1)
        {
          scaleValue = pFilteredRows1[x];
        }
        else
        {
          scaleValue = pFilteredRows2[x];
        }
        
        //} // for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; scaleImage_numPyramidLevels++)
        
        const s32 thresholdValue = (scaleValue*scaleImage_thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;
        if(pImage[x] < thresholdValue)
        {
          pBinaryImageRow[x] = isDarkOnLight;
        } 
        else 
        {
          pBinaryImageRow[x] = !isDarkOnLight;
        }
      } // for(s32 x=0; x<imageWidth; x++)
#endif
    } // staticInline void ecvcs_computeBinaryImage_numFilters5()
    
    NO_INLINE void ecvcs_computeBinaryImage_numFilters5(const Array<u8> &image, FixedLengthList<Array<u8> > &filteredRows, const s32 scaleImage_thresholdMultiplier, const s32 imageY, u8 * restrict pBinaryImageRow, const bool isDarkOnLight)
    {
      AnkiAssert(filteredRows.get_size() == 5);

      const s32 thresholdMultiplier_numFractionalBits = 16;

      const u8 * restrict pImage = image[imageY];

      const u8 * restrict pFilteredRows0 = filteredRows[0][0];
      const u8 * restrict pFilteredRows1 = filteredRows[1][0];
      const u8 * restrict pFilteredRows2 = filteredRows[2][0];
      const u8 * restrict pFilteredRows3 = filteredRows[3][0];
      const u8 * restrict pFilteredRows4 = filteredRows[4][0];

      const s32 imageWidth = image.get_size(1);

      const s32 numFilteredRows = filteredRows.get_size();

      AnkiAssert(filteredRows.get_size() <= MAX_FILTER_HALF_WIDTH);

      const u8 * restrict pFilteredRows[MAX_FILTER_HALF_WIDTH+1];
      for(s32 i=0; i<numFilteredRows; i++) {
        pFilteredRows[i] = filteredRows[i][0];
      }

      for(s32 x=0; x<imageWidth; x++) {
        //for(s32 iHalfWidth=0; iHalfWidth<(numFilteredRows-1); iHalfWidth++) {
        const s32 dog0 = ABS(static_cast<s32>(pFilteredRows1[x]) - static_cast<s32>(pFilteredRows0[x]));
        const s32 dog1 = ABS(static_cast<s32>(pFilteredRows2[x]) - static_cast<s32>(pFilteredRows1[x]));
        const s32 dog2 = ABS(static_cast<s32>(pFilteredRows3[x]) - static_cast<s32>(pFilteredRows2[x]));
        const s32 dog3 = ABS(static_cast<s32>(pFilteredRows4[x]) - static_cast<s32>(pFilteredRows3[x]));

        const s32 maxValue = MAX(dog0, MAX(dog1, MAX(dog2, dog3)));

        s32 scaleValue;

        if(dog0 == maxValue) {
          scaleValue = pFilteredRows1[x];
        } else if(dog1 == maxValue) {
          scaleValue = pFilteredRows2[x];
        } else if(dog2 == maxValue) {
          scaleValue = pFilteredRows3[x];
        } else {
          scaleValue = pFilteredRows4[x];
        }

        //} // for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; scaleImage_numPyramidLevels++)

        const s32 thresholdValue = (scaleValue*scaleImage_thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;
        if(pImage[x] < thresholdValue) {
          pBinaryImageRow[x] = isDarkOnLight;
        } else {
          pBinaryImageRow[x] = !isDarkOnLight;
        }
      } // for(s32 x=0; x<imageWidth; x++)
    } // staticInline void ecvcs_computeBinaryImage_numFilters5()

    NO_INLINE void ecvcs_computeBinaryImage_numFilters5_thresholdMultiplier1(const Array<u8> &image, FixedLengthList<Array<u8> > &filteredRows, const s32 scaleImage_thresholdMultiplier, const s32 imageY, u8 * restrict pBinaryImageRow, const bool isDarkOnLight)
    {
      AnkiAssert(filteredRows.get_size() == 5);
      AnkiAssert(scaleImage_thresholdMultiplier == 65536);

      //const s32 thresholdMultiplier_numFractionalBits = 16;

      const u8 * restrict pImage = image[imageY];

      const u8 * restrict pFilteredRows0 = filteredRows[0][0];
      const u8 * restrict pFilteredRows1 = filteredRows[1][0];
      const u8 * restrict pFilteredRows2 = filteredRows[2][0];
      const u8 * restrict pFilteredRows3 = filteredRows[3][0];
      const u8 * restrict pFilteredRows4 = filteredRows[4][0];

      const s32 imageWidth = image.get_size(1);

      const s32 numFilteredRows = filteredRows.get_size();

      AnkiAssert(filteredRows.get_size() <= MAX_FILTER_HALF_WIDTH);

      const u8 * restrict pFilteredRows[MAX_FILTER_HALF_WIDTH+1];
      for(s32 i=0; i<numFilteredRows; i++) {
        pFilteredRows[i] = filteredRows[i][0];
      }

      s32 x=0;

#if ACCELERATION_TYPE == ACCELERATION_ARM_A7
      const uint8x16_t zeros8x16 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      const uint8x16_t ones8x16  = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

      for(; x<(imageWidth-15); x+=16) {
        const uint8x16_t filteredRow0 = vld1q_u8(&pFilteredRows0[x]);
        const uint8x16_t filteredRow1 = vld1q_u8(&pFilteredRows1[x]);
        const uint8x16_t filteredRow2 = vld1q_u8(&pFilteredRows2[x]);
        const uint8x16_t filteredRow3 = vld1q_u8(&pFilteredRows3[x]);
        const uint8x16_t filteredRow4 = vld1q_u8(&pFilteredRows4[x]);

        const uint8x16_t imageRow = vld1q_u8(&pImage[x]);

        const uint8x16_t dog0 = vabdq_u8(filteredRow1, filteredRow0);
        const uint8x16_t dog1 = vabdq_u8(filteredRow2, filteredRow1);
        const uint8x16_t dog2 = vabdq_u8(filteredRow3, filteredRow2);
        const uint8x16_t dog3 = vabdq_u8(filteredRow4, filteredRow3);

        const uint8x16_t dogMax = vmaxq_u8(vmaxq_u8(dog0, dog1), vmaxq_u8(dog2, dog3));

        const uint8x16_t dog0IsMax = vceqq_u8(dogMax, dog0);
        const uint8x16_t dog1IsMax = vceqq_u8(dogMax, dog1);
        const uint8x16_t dog2IsMax = vceqq_u8(dogMax, dog2);

        // Backwards, so in cases of ties, the result matches the non-simd version
        uint8x16_t scaleValue = filteredRow4;
        scaleValue = vbslq_u8(dog2IsMax, filteredRow3, scaleValue);
        scaleValue = vbslq_u8(dog1IsMax, filteredRow2, scaleValue);
        scaleValue = vbslq_u8(dog0IsMax, filteredRow1, scaleValue);

        const uint8x16_t scaleValueIsLarger = vcltq_u8(imageRow, scaleValue);
        const uint8x16_t binaryVector = vbslq_u8(scaleValueIsLarger, ones8x16, zeros8x16);

        vst1q_u8(&pBinaryImageRow[x], binaryVector);
      } // for(s32 x=0; x<imageWidth; x++)
#else // #if ACCELERATION_TYPE == ACCELERATION_ARM_A7

      for(; x<imageWidth; x++) {
        //for(s32 iHalfWidth=0; iHalfWidth<(numFilteredRows-1); iHalfWidth++) {
        const s16 dog0 = ABS(static_cast<s16>(pFilteredRows1[x]) - static_cast<s16>(pFilteredRows0[x]));
        const s16 dog1 = ABS(static_cast<s16>(pFilteredRows2[x]) - static_cast<s16>(pFilteredRows1[x]));
        const s16 dog2 = ABS(static_cast<s16>(pFilteredRows3[x]) - static_cast<s16>(pFilteredRows2[x]));
        const s16 dog3 = ABS(static_cast<s16>(pFilteredRows4[x]) - static_cast<s16>(pFilteredRows3[x]));

        const s16 maxValue = MAX(dog0, MAX(dog1, MAX(dog2, dog3)));

        u8 scaleValue;

        if(dog0 == maxValue) {
          scaleValue = pFilteredRows1[x];
        } else if(dog1 == maxValue) {
          scaleValue = pFilteredRows2[x];
        } else if(dog2 == maxValue) {
          scaleValue = pFilteredRows3[x];
        } else {
          scaleValue = pFilteredRows4[x];
        }

        //} // for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; scaleImage_numPyramidLevels++)

        const u8 thresholdValue = scaleValue;
        if(pImage[x] < thresholdValue) {
          pBinaryImageRow[x] = isDarkOnLight;
        } else {
          pBinaryImageRow[x] = !isDarkOnLight;
        }
      } // for(s32 x=0; x<imageWidth; x++)
#endif
    } // staticInline void ecvcs_computeBinaryImage_numFilters5()

    NO_INLINE void ecvcs_computeBinaryImage(const Array<u8> &image, FixedLengthList<Array<u8> > &filteredRows, const s32 scaleImage_thresholdMultiplier, const s32 imageY, u8 * restrict pBinaryImageRow, const bool isDarkOnLight)
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
        s32 dogMax = std::numeric_limits<s32>::min();
        for(s32 iHalfWidth=0; iHalfWidth<(numFilteredRows-1); iHalfWidth++) {
          const s32 dog = ABS(static_cast<s32>(pFilteredRows[iHalfWidth+1][x]) - static_cast<s32>(pFilteredRows[iHalfWidth][x]));

          if(dog > dogMax) {
            dogMax = dog;
            scaleValue = pFilteredRows[iHalfWidth+1][x];
          } // if(dog > dogMax)
        } // for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; scaleImage_numPyramidLevels++)

        const s32 thresholdValue = (scaleValue*scaleImage_thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;
        if(pImage[x] < thresholdValue) {
          pBinaryImageRow[x] = isDarkOnLight;
        } else {
          pBinaryImageRow[x] = !isDarkOnLight;
        }
      } // for(s32 x=0; x<imageWidth; x++)
    } // staticInline void ecvcs_computeBinaryImage()

    Result ExtractComponentsViaCharacteristicScale(
      const Array<u8> &image,
      const FixedLengthList<s32> &filterHalfWidths, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const bool isDarkOnLight,
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
      for(s32 i=0; i<numFilterHalfWidths; i++) {
        AnkiAssert(pFilterHalfWidths[i] >= 0);
        AnkiAssert(pFilterHalfWidths[i] <= MAX_FILTER_HALF_WIDTH);
        numBorderPixels = MAX(numBorderPixels, pFilterHalfWidths[i] + 1);
      }

      // NOTE: This is old embedded code from a time when memory was limited and the
      // entire integral image could not be held in memory. So it had to be
      // computed in a scrolling fashion. On a phone/device, this is not an issue
      // so we just set the integralImageHeight to be the imageHeight and call it a
      //
      //      // TODO: choose based on amount of free memory
      //      if(imageWidth <= 320) {
      //        integralImageHeight = MAX(2*numBorderPixels + 16, 64); // 96*(640+33*2)*4 = 271104, though with padding, it is 96*720*4 = 276480
      //      } else {
      //        // Note: These numbers are liable to be too big to fit on the M4 efficiently
      //        const s32 scaleFactor = static_cast<s32>(ceilf(static_cast<f32>(imageWidth) / 320.0f));
      //        integralImageHeight = MAX(2*numBorderPixels + 16*scaleFactor, 64*scaleFactor);
      //      }
      const s32 integralImageHeight = imageHeight;

      const s32 numRowsToScroll = integralImageHeight - 2*numBorderPixels;

      AnkiAssert(numRowsToScroll > 1);

      // Initialize the first integralImageHeight rows of the integralImage
      ScrollingIntegralImage_u8_s32 integralImage(integralImageHeight, imageWidth, numBorderPixels, slowerScratch);
      if((lastResult = integralImage.ScrollDown(image, integralImageHeight, fastScratch)) != RESULT_OK)
        return lastResult;
      
      //      // Trying to use OpenCV's integral image
      //      cv::Mat_<u8> cvImg;
      //      ArrayToCvMat(image, &cvImg);
      //      static cv::Mat_<s32> cvCompute, cvIntImg;
      //      ArrayToCvMat(integralImage, &cvIntImg);
      //      cv::integral(cvImg, cvCompute, CV_32S);
      //      cv::copyMakeBorder(cvCompute(cv::Rect(1,1,imageWidth,integralImageHeight)), cvIntImg, 0, 0, numBorderPixels, numBorderPixels, cv::BORDER_REPLICATE);
      
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

#if STORE_BINARY_IMAGE
      // TODO: change resolution if the input resolution changes
      if(!g_saveBinaryInitialized) {
        MemoryStack tmpMemory(&binaryImageBuffer[0], binaryImageBufferLength);
        g_binaryImage = Anki::Embedded::Array<u8>(image.get_size(0), image.get_size(1), tmpMemory);
        g_saveBinaryInitialized = true;
      }
#endif

      std::function<void(const Array<u8>&, FixedLengthList<Array<u8>>&, const s32, const s32, u8 * restrict, const bool)> binarizationFcn = nullptr;
      
      switch(numFilterHalfWidths)
      {
        case 3:
          binarizationFcn = &ecvcs_computeBinaryImage_numFilters3;
          break;
          
        case 5:
          if(scaleImage_thresholdMultiplier == 65536) {
            binarizationFcn = &ecvcs_computeBinaryImage_numFilters5_thresholdMultiplier1;
          } else {
            binarizationFcn = &ecvcs_computeBinaryImage_numFilters5;
          }
          break;
          
        default:
          binarizationFcn = &ecvcs_computeBinaryImage;
          break;
      }
      
      BeginBenchmark("ecvcs_mainLoop");
      while(imageY < imageHeight) {
        BeginBenchmark("ecvcs_filterRows");
        ecvcs_filterRows(integralImage, filterHalfWidths, imageY, filteredRows);
        EndBenchmark("ecvcs_filterRows");

        BeginBenchmark("ecvcs_computeBinaryImage");

        binarizationFcn(image, filteredRows, scaleImage_thresholdMultiplier, imageY, pBinaryImageRow, isDarkOnLight);
        
#if STORE_BINARY_IMAGE
        memcpy(g_binaryImage.Pointer(imageY, 0), pBinaryImageRow, imageWidth);
#endif

        EndBenchmark("ecvcs_computeBinaryImage");

        // Extract the next line of connected components
        BeginBenchmark("ecvcs_extractNextRowOfComponents");
        if((lastResult = components.Extract2dComponents_PerRow_NextRow(pBinaryImageRow, imageWidth, imageY, component1d_minComponentWidth, component1d_maxSkipDistance)) != RESULT_OK)
          return lastResult;
        EndBenchmark("ecvcs_extractNextRowOfComponents");

        BeginBenchmark("ecvcs_scrollIntegralImage");

        imageY++;

        // If we've reached the bottom of this integral image, scroll it up
        if(integralImage.get_maxRow(numBorderPixels-1) < imageY) {
          if((lastResult = integralImage.ScrollDown(image, numRowsToScroll, fastScratch)) != RESULT_OK)
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

