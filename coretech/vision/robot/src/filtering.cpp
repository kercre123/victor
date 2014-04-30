/**
File: filtering.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/imageProcessing.h"

#include "anki/common/constantsAndMacros.h"

//using namespace std;

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
      typedef enum
      {
        BITSHIFT_NONE,
        BITSHIFT_LEFT,
        BITSHIFT_RIGHT
      } BitShiftDirection;

      static void GetBitShiftDirectionAndMagnitude(
        const s32 in1_numFractionalBits, const s32 in2_numFractionalBit, const s32 out_numFractionalBit,
        s32 &shiftMagnitude, BitShiftDirection &shiftDirection)
      {
        if((in1_numFractionalBits+in2_numFractionalBit) > out_numFractionalBit) {
          shiftDirection = BITSHIFT_RIGHT;
          shiftMagnitude = in1_numFractionalBits + in2_numFractionalBit - out_numFractionalBit;
        } else if((in1_numFractionalBits+in2_numFractionalBit) < out_numFractionalBit) {
          shiftDirection = BITSHIFT_LEFT;
          shiftMagnitude = out_numFractionalBit - in1_numFractionalBits - in2_numFractionalBit;
        } else {
          shiftDirection = BITSHIFT_NONE;
          shiftMagnitude = 0;
        }
      } // static void GetBitShiftDirectionAndMagnitude()

      FixedPointArray<s32> Get1dGaussianKernel(const s32 sigma, const s32 numSigmaFractionalBits, const s32 numStandardDeviations, MemoryStack &scratch)
      {
        // halfWidth = ceil(num_std*sigma);
        const s32 halfWidth = 1 + ((sigma*numStandardDeviations) >> numSigmaFractionalBits);
        const f32 halfWidthF32 = static_cast<f32>(halfWidth);

        FixedPointArray<s32> gaussianKernel(1, 2*halfWidth + 1, numSigmaFractionalBits, scratch, Flags::Buffer(false,false,false));
        s32 * restrict pGaussianKernel = gaussianKernel.Pointer(0,0);

        {
          PUSH_MEMORY_STACK(scratch);

          Array<f32> gaussianKernelF32(1, 2*halfWidth + 1, scratch);
          f32 * restrict pGaussianKernelF32 = gaussianKernelF32.Pointer(0,0);

          const f32 twoTimesSigmaSquared = static_cast<f32>(2*sigma*sigma) / powf(2.0f, static_cast<f32>(numSigmaFractionalBits*2));

          s32 i = 0;
          f32 sum = 0;
          for(f32 x=-halfWidthF32; i<(2*halfWidth+1); x++, i++) {
            const f32 g = expf(-(x*x) / twoTimesSigmaSquared);
            pGaussianKernelF32[i] = g;
            sum += g;
          }

          // Normalize to sum to one
          const f32 sumInverse = 1.0f / sum;
          const f32 twoToNumBits = powf(2.0f, static_cast<f32>(numSigmaFractionalBits));
          for(s32 i=0; i<(2*halfWidth+1); i++) {
            const f32 gScaled = pGaussianKernelF32[i] * sumInverse * twoToNumBits;
            pGaussianKernel[i] = Round<s32>(gScaled);
          }

          // gaussianKernelF32.Print("gaussianKernelF32");
        } // PUSH_MEMORY_STACK(scratch);

        // gaussianKernel.Print("gaussianKernel");

        return gaussianKernel;
      }

      // NOTE: uses a 32-bit accumulator, so be careful of overflows
      Result Correlate1d(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)
      {
        const s32 outputLength = in1.get_size(1) + in2.get_size(1) - 1;

        AnkiConditionalErrorAndReturnValue(in1.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "in1 is not valid");

        AnkiConditionalErrorAndReturnValue(in2.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "in2 is not valid");

        AnkiConditionalErrorAndReturnValue(out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "out is not valid");

        AnkiConditionalErrorAndReturnValue(in1.get_size(0)==1 && in2.get_size(0)==1 && out.get_size(0)==1,
          RESULT_FAIL_INVALID_SIZE, "ComputeQuadrilateralsFromConnectedComponents", "Arrays must be 1d and horizontal");

        AnkiConditionalErrorAndReturnValue(out.get_size(1) == outputLength,
          RESULT_FAIL_INVALID_SIZE, "ComputeQuadrilateralsFromConnectedComponents", "Out must be the size of in1 + in2 - 1");

        AnkiConditionalErrorAndReturnValue(in1.get_rawDataPointer() != in2.get_rawDataPointer() && in1.get_rawDataPointer() != out.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "ComputeQuadrilateralsFromConnectedComponents", "in1, in2, and out must be in different memory locations");

        const s32 * restrict pU;
        const s32 * restrict pV;
        s32 * restrict pOut = out.Pointer(0,0);

        // To simplify things, u is the longer of the two,
        // and v is the shorter of the two
        s32 uLength;
        s32 vLength;
        if(in1.get_size(1) > in2.get_size(1)) {
          pU = in1.Pointer(0,0);
          pV = in2.Pointer(0,0);
          uLength = in1.get_size(1);
          vLength = in2.get_size(1);
        } else {
          pU = in2.Pointer(0,0);
          pV = in1.Pointer(0,0);
          uLength = in2.get_size(1);
          vLength = in1.get_size(1);
        }

        const s32 midStartIndex = vLength - 1;
        const s32 midEndIndex = midStartIndex + uLength - vLength;

        s32 shiftMagnitude;
        BitShiftDirection shiftDirection;
        GetBitShiftDirectionAndMagnitude(in1.get_numFractionalBits(), in2.get_numFractionalBits(), out.get_numFractionalBits(), shiftMagnitude, shiftDirection);

        s32 iOut = 0;

        // Filter the left part
        for(s32 x=0; x<midStartIndex; x++) {
          s32 sum = 0;
          for(s32 xx=0; xx<=x; xx++) {
            const s32 toAdd = (pU[xx] * pV[vLength-x+xx-1]);
            sum += toAdd;
          }

          // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
          if(shiftDirection == BITSHIFT_RIGHT) {
            pOut[iOut++] = sum >> shiftMagnitude;
          } else if(shiftDirection == BITSHIFT_LEFT) {
            pOut[iOut++] = sum << shiftMagnitude;
          } else {
            pOut[iOut++] = sum;
          }
        }

        // Filter the middle part
        for(s32 x=midStartIndex; x<=midEndIndex; x++) {
          s32 sum = 0;
          for(s32 xx=0; xx<vLength; xx++) {
            const s32 toAdd = (pU[x+xx-midStartIndex] * pV[xx]);
            sum += toAdd;
          }

          // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
          if(shiftDirection == BITSHIFT_RIGHT) {
            pOut[iOut++] = sum >> shiftMagnitude;
          } else if(shiftDirection == BITSHIFT_LEFT) {
            pOut[iOut++] = sum << shiftMagnitude;
          } else {
            pOut[iOut++] = sum;
          }
        }

        // Filter the right part
        for(s32 x=midEndIndex+1; x<outputLength; x++) {
          const s32 vEnd = outputLength - x;
          s32 sum = 0;
          for(s32 xx=0; xx<vEnd; xx++) {
            const s32 toAdd = (pU[x+xx-midStartIndex] * pV[xx]);
            sum += toAdd;
          }

          // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
          if(shiftDirection == BITSHIFT_RIGHT) {
            pOut[iOut++] = sum >> shiftMagnitude;
          } else if(shiftDirection == BITSHIFT_LEFT) {
            pOut[iOut++] = sum << shiftMagnitude;
          } else {
            pOut[iOut++] = sum;
          }
        }

        return RESULT_OK;
      } // Result Correlate1d(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)

      // NOTE: uses a 32-bit accumulator, so be careful of overflows
      Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &image, const FixedPointArray<s32> &filter, FixedPointArray<s32> &out)
      {
        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        const s32 filterHeight = filter.get_size(0);
        const s32 filterWidth = filter.get_size(1);

        AnkiConditionalErrorAndReturnValue(image.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "image is not valid");

        AnkiConditionalErrorAndReturnValue(filter.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "filter is not valid");

        AnkiConditionalErrorAndReturnValue(out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "out is not valid");

        AnkiConditionalErrorAndReturnValue(imageHeight==1 && filterHeight==1 && out.get_size(0)==1,
          RESULT_FAIL_INVALID_SIZE, "ComputeQuadrilateralsFromConnectedComponents", "Arrays must be 1d and horizontal");

        AnkiConditionalErrorAndReturnValue(imageWidth > filterWidth,
          RESULT_FAIL_INVALID_SIZE, "ComputeQuadrilateralsFromConnectedComponents", "The image must be larger than the filter");

        AnkiConditionalErrorAndReturnValue(image.get_rawDataPointer() != filter.get_rawDataPointer() && image.get_rawDataPointer() != out.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "ComputeQuadrilateralsFromConnectedComponents", "in1, in2, and out must be in different memory locations");

        const s32 * restrict pImage = image.Pointer(0,0);
        const s32 * restrict pFilter = filter.Pointer(0,0);
        s32 * restrict pOut = out.Pointer(0,0);

        s32 shiftMagnitude;
        BitShiftDirection shiftDirection;
        GetBitShiftDirectionAndMagnitude(image.get_numFractionalBits(), filter.get_numFractionalBits(), out.get_numFractionalBits(), shiftMagnitude, shiftDirection);

        //const s32 filterWidth = filterWidth;
        const s32 filterHalfWidth = filterWidth >> 1;

        //image.Print("image");
        //filter.Print("filter");

        // Filter the middle part
        for(s32 x=0; x<imageWidth; x++) {
          s32 sum = 0;
          for(s32 xFilter=0; xFilter<filterWidth; xFilter++) {
            // TODO: if this is too slow, pull out of the loop
            s32 xImage = (x - filterHalfWidth + xFilter);
            //s32 xImage = (x - filterWidth + xFilter + 1);
            if(xImage < 0) {
              xImage += imageWidth;
            } else if(xImage >= imageWidth) {
              xImage -= imageWidth;
            }

            const s32 toAdd = pImage[xImage] * pFilter[xFilter];
            sum += toAdd;
          }

          // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
          if(shiftDirection == BITSHIFT_RIGHT) {
            pOut[x] = sum >> shiftMagnitude;
          } else if(shiftDirection == BITSHIFT_LEFT) {
            pOut[x] = sum << shiftMagnitude;
          } else {
            pOut[x] = sum;
          }
        }

        return RESULT_OK;
      } // Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)
      
      /*
      // This is a general function with specializations defined for relevant
      // types
      template<typename Type> Type GetImageTypeMean(void);
      
      // TODO: these should be in a different .cpp file (imageProcessing.cpp?)
      template<> f32 GetImageTypeMean<f32>() { return 0.5f; }
      template<> u8  GetImageTypeMean<u8>()  { return 128;  }
      template<> f64 GetImageTypeMean<f64>() { return 0.5f; }
     */
      
      
      
      Result BoxFilterNormalize(const Array<u8> &image, const s32 boxSize, const u8 padValue,
                                Array<u8> &imageNorm, MemoryStack scratch)
      {
        Result lastResult = RESULT_OK;
        
        AnkiConditionalErrorAndReturnValue(image.IsValid(),
                                           RESULT_FAIL_INVALID_OBJECT,
                                           "BoxFilterNormalize",
                                           "Input image is invalid.");
        
        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth  = image.get_size(1);
        
        AnkiConditionalErrorAndReturnValue(imageNorm.IsValid(),
                                           RESULT_FAIL_INVALID_OBJECT,
                                           "BoxFilterNormalize",
                                           "Output normalized image is invalid.");
        
        AnkiConditionalErrorAndReturnValue(imageNorm.get_size(0) == imageHeight &&
                                           imageNorm.get_size(1) == imageWidth,
                                           RESULT_FAIL_INVALID_SIZE,
                                           "BoxFilterNormalize",
                                           "Output normalized image must match input image's size.");
        
        Array<f32> integralImage(imageHeight, imageWidth, scratch);
        
        AnkiConditionalErrorAndReturnValue(integralImage.IsValid(),
                                           RESULT_FAIL_OUT_OF_MEMORY,
                                           "BoxFilterNormalize",
                                           "Could not allocate integral image (out of memory?).");
        
        if((lastResult = CreateIntegralImage(image, integralImage)) != RESULT_OK) {
          return lastResult;
        }
        
        // Divide each input pixel by box filter results computed from integral image
        const s32 halfWidth = MIN(MIN(imageWidth,imageHeight)/2, boxSize/2);
        const s32 boxWidth  = 2*halfWidth + 1;
        const f32 boxArea   = static_cast<f32>(boxWidth*boxWidth);
        const f32 outMean = 128.f; //static_cast<f32>(GetImageTypeMean<u8>());
        
        for(s32 y=0; y<imageHeight; y++) {
          // Input/Output pixel pointers:
          const u8 * restrict pImageRow     = image.Pointer(y,0);
          u8       * restrict pImageRowNorm = imageNorm.Pointer(y,0);
          
          // Integral image pointers for top and bottom of summing box
          s32 rowAhead  = y + halfWidth;
          s32 rowBehind = y - halfWidth - 1;
          s32 inBoundsHeight = boxWidth;
          if(rowAhead >= imageHeight) {
            inBoundsHeight = imageHeight - y + halfWidth;
            rowAhead = imageHeight - 1;
          }
          if(rowBehind < 0) {
            inBoundsHeight = y+halfWidth+1;
            rowBehind = 0;
          }
          
          const f32 * restrict pIntegralImageRowBehind = integralImage.Pointer(rowBehind,0);
          const f32 * restrict pIntegralImageRowAhead  = integralImage.Pointer(rowAhead, 0);
          
          // Left side
          for(s32 x=0; x<=halfWidth; x++) {
            f32 OutOfBoundsArea = static_cast<f32>(boxArea - (x+halfWidth+1)*inBoundsHeight);
            
            f32 boxSum = (pIntegralImageRowAhead[x+halfWidth] -
                          pIntegralImageRowAhead[0] -
                          pIntegralImageRowBehind[x+halfWidth] +
                          pIntegralImageRowBehind[0] +
                          OutOfBoundsArea*static_cast<f32>(padValue));
            
            pImageRowNorm[x] = static_cast<u8>(CLIP(outMean * (static_cast<f32>(pImageRow[x]) * boxArea) / boxSum, 0.f, 255.f));
          }

          // Middle
          const f32 OutOfBoundsArea = static_cast<f32>(boxArea - boxWidth*inBoundsHeight);
          const f32 paddingSum = OutOfBoundsArea * static_cast<f32>(padValue);
          for(s32 x=halfWidth+1; x<imageWidth-halfWidth-1; x++) {
            f32 boxSum = (pIntegralImageRowAhead[x+halfWidth] -
                          pIntegralImageRowAhead[x-halfWidth-1] -
                          pIntegralImageRowBehind[x+halfWidth] +
                          pIntegralImageRowBehind[x-halfWidth-1] +
                          paddingSum);
            
            pImageRowNorm[x] = static_cast<u8>(CLIP(outMean * (static_cast<f32>(pImageRow[x]) * boxArea) / boxSum, 0.f, 255.f));
          }
          
          // Right side
          for(s32 x=imageWidth-halfWidth-1; x<imageWidth; x++) {
            f32 OutOfBoundsArea = static_cast<f32>(boxArea - (imageWidth-x+halfWidth)*inBoundsHeight);
            
            f32 boxSum = (pIntegralImageRowAhead[imageWidth-1] -
                          pIntegralImageRowAhead[x-halfWidth-1] -
                          pIntegralImageRowBehind[imageWidth-1] +
                          pIntegralImageRowBehind[x-halfWidth-1] +
                          OutOfBoundsArea*static_cast<f32>(padValue));
            
            pImageRowNorm[x] = static_cast<u8>(CLIP(outMean * (static_cast<f32>(pImageRow[x]) * boxArea) / boxSum, 0.f, 255.f));
          }
        }
        
        return RESULT_OK;
        
      } // BoxFilterNormalize()
      
      
    } // namespace ImageProcessing
  } // namespace Embedded
} // namespace Anki
