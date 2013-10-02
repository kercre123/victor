#include "anki/embeddedVision/miscVisionKernels.h"

using namespace std;

namespace Anki
{
  namespace Embedded
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
      // halfwidth = ceil(num_std*sigma);
      const s32 halfwidth = 1 + ((sigma*numStandardDeviations) >> numSigmaFractionalBits);
      const f32 halfwidthF32 = static_cast<f32>(halfwidth);

      FixedPointArray<s32> gaussianKernel(1, 2*halfwidth + 1, numSigmaFractionalBits, scratch);
      s32 * restrict gaussianKernel_rowPointer = gaussianKernel.Pointer(0,0);

      {
        PUSH_MEMORY_STACK(scratch);

        Array<f32> gaussianKernelF32(1, 2*halfwidth + 1, scratch);
        f32 * restrict gaussianKernelF32_rowPointer = gaussianKernelF32.Pointer(0,0);

        const f32 twoTimesSigmaSquared = static_cast<f32>(2*sigma*sigma) / powf(2.0f, static_cast<f32>(numSigmaFractionalBits*2));

        s32 i = 0;
        f32 sum = 0;
        for(f32 x=-halfwidthF32; i<(2*halfwidth+1); x++, i++) {
          const f32 g = exp(-(x*x) / twoTimesSigmaSquared);
          gaussianKernelF32_rowPointer[i] = g;
          sum += g;
        }

        // Normalize to sum to one
        const f32 sumInverse = 1.0f / sum;
        for(s32 i=0; i<(2*halfwidth+1); i++) {
          const f32 gScaled = gaussianKernelF32_rowPointer[i] * sumInverse * powf(2.0f, static_cast<f32>(numSigmaFractionalBits));
          gaussianKernel_rowPointer[i] = static_cast<s32>(Round(gScaled));
        }

        // gaussianKernelF32.Print("gaussianKernelF32");
      } // PUSH_MEMORY_STACK(scratch);

      // gaussianKernel.Print("gaussianKernel");

      return gaussianKernel;
    }

    // Note: uses a 32-bit accumulator, so be careful of overflows
    Result Correlate1d(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)
    {
      const s32 outputLength = in1.get_size(1) + in2.get_size(1) - 1;

      AnkiConditionalErrorAndReturnValue(in1.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "in1 is not valid");

      AnkiConditionalErrorAndReturnValue(in2.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "in2 is not valid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "out is not valid");

      AnkiConditionalErrorAndReturnValue(in1.get_size(0)==1 && in2.get_size(0)==1 && out.get_size(0)==1,
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "Arrays must be 1d and horizontal");

      AnkiConditionalErrorAndReturnValue(out.get_size(1) == outputLength,
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "Out must be the size of in1 + in2 - 1");

      AnkiConditionalErrorAndReturnValue(in1.get_rawDataPointer() != in2.get_rawDataPointer() && in1.get_rawDataPointer() != out.get_rawDataPointer(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "in1, in2, and out must be in different memory locations");

      const s32 * restrict u_rowPointer;
      const s32 * restrict v_rowPointer;
      s32 * restrict out_rowPointer = out.Pointer(0,0);

      // To simplify things, u is the longer of the two,
      // and v is the shorter of the two
      s32 uLength;
      s32 vLength;
      if(in1.get_size(1) > in2.get_size(1)) {
        u_rowPointer = in1.Pointer(0,0);
        v_rowPointer = in2.Pointer(0,0);
        uLength = in1.get_size(1);
        vLength = in2.get_size(1);
      } else {
        u_rowPointer = in2.Pointer(0,0);
        v_rowPointer = in1.Pointer(0,0);
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
          const s32 toAdd = (u_rowPointer[xx] * v_rowPointer[vLength-x+xx-1]);
          sum += toAdd;
        }

        // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
        if(shiftDirection == BITSHIFT_RIGHT) {
          out_rowPointer[iOut++] = sum >> shiftMagnitude;
        } else if(shiftDirection == BITSHIFT_LEFT) {
          out_rowPointer[iOut++] = sum << shiftMagnitude;
        } else {
          out_rowPointer[iOut++] = sum;
        }
      }

      // Filter the middle part
      for(s32 x=midStartIndex; x<=midEndIndex; x++) {
        s32 sum = 0;
        for(s32 xx=0; xx<vLength; xx++) {
          const s32 toAdd = (u_rowPointer[x+xx-midStartIndex] * v_rowPointer[xx]);
          sum += toAdd;
        }

        // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
        if(shiftDirection == BITSHIFT_RIGHT) {
          out_rowPointer[iOut++] = sum >> shiftMagnitude;
        } else if(shiftDirection == BITSHIFT_LEFT) {
          out_rowPointer[iOut++] = sum << shiftMagnitude;
        } else {
          out_rowPointer[iOut++] = sum;
        }
      }

      // Filter the right part
      for(s32 x=midEndIndex+1; x<outputLength; x++) {
        const s32 vEnd = outputLength - x;
        s32 sum = 0;
        for(s32 xx=0; xx<vEnd; xx++) {
          const s32 toAdd = (u_rowPointer[x+xx-midStartIndex] * v_rowPointer[xx]);
          sum += toAdd;
        }

        // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
        if(shiftDirection == BITSHIFT_RIGHT) {
          out_rowPointer[iOut++] = sum >> shiftMagnitude;
        } else if(shiftDirection == BITSHIFT_LEFT) {
          out_rowPointer[iOut++] = sum << shiftMagnitude;
        } else {
          out_rowPointer[iOut++] = sum;
        }
      }

      return RESULT_OK;
    } // Result Correlate1d(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)

    // Note: uses a 32-bit accumulator, so be careful of overflows
    Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &image, const FixedPointArray<s32> &filter, FixedPointArray<s32> &out)
    {
      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "image is not valid");

      AnkiConditionalErrorAndReturnValue(filter.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "filter is not valid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "out is not valid");

      AnkiConditionalErrorAndReturnValue(image.get_size(0)==1 && filter.get_size(0)==1 && out.get_size(0)==1,
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "Arrays must be 1d and horizontal");

      AnkiConditionalErrorAndReturnValue(image.get_size(1) > filter.get_size(1),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "The image must be larger than the filter");

      AnkiConditionalErrorAndReturnValue(image.get_rawDataPointer() != filter.get_rawDataPointer() && image.get_rawDataPointer() != out.get_rawDataPointer(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "in1, in2, and out must be in different memory locations");

      const s32 * restrict image_rowPointer = image.Pointer(0,0);
      const s32 * restrict filter_rowPointer = filter.Pointer(0,0);
      s32 * restrict out_rowPointer = out.Pointer(0,0);

      s32 shiftMagnitude;
      BitShiftDirection shiftDirection;
      GetBitShiftDirectionAndMagnitude(image.get_numFractionalBits(), filter.get_numFractionalBits(), out.get_numFractionalBits(), shiftMagnitude, shiftDirection);

      //const s32 filterWidth = filter.get_size(1);
      const s32 filterHalfWidth = filter.get_size(1) >> 1;

      //image.Print("image");
      //filter.Print("filter");

      // Filter the middle part
      for(s32 x=0; x<image.get_size(1); x++) {
        s32 sum = 0;
        for(s32 xFilter=0; xFilter<filter.get_size(1); xFilter++) {
          // TODO: if this is too slow, pull out of the loop
          s32 xImage = (x - filterHalfWidth + xFilter);
          //s32 xImage = (x - filterWidth + xFilter + 1);
          if(xImage < 0) {
            xImage += image.get_size(1);
          } else if(xImage >= image.get_size(1)) {
            xImage -= image.get_size(1);
          }

          const s32 toAdd = image_rowPointer[xImage] * filter_rowPointer[xFilter];
          sum += toAdd;
        }

        // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
        if(shiftDirection == BITSHIFT_RIGHT) {
          out_rowPointer[x] = sum >> shiftMagnitude;
        } else if(shiftDirection == BITSHIFT_LEFT) {
          out_rowPointer[x] = sum << shiftMagnitude;
        } else {
          out_rowPointer[x] = sum;
        }
      }

      return RESULT_OK;
    } // Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out)

    Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, FixedLengthList<Point<s16> > &peaks, MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(boundary.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "boundary is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "peaks is not valid");

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.get_maximumSize() == 4,
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "Currently only four peaks supported");

      const s32 maximumTemporaryPeaks = boundary.get_size() / 3; // Worst case
      const s32 numSigmaFractionalBits = 8;
      const s32 numStandardDeviations = 3;

      const s32 boundaryLength = boundary.get_size();

      //sigma = boundaryLength/64;
      const s32 sigma = boundaryLength << (numSigmaFractionalBits-6); // SQ23.8

      // spacing about 1/4 of side-length
      //spacing = max(3, round(boundaryLength/16));
      const s32 spacing = MAX(3,  boundaryLength >> 4);

      //  stencil = [1 zeros(1, spacing-2) -2 zeros(1, spacing-2) 1];
      const s32 stencilLength = 1 + spacing - 2 + 1 + spacing - 2 + 1;
      FixedPointArray<s32> stencil(1, stencilLength, 0, scratch); // SQ31.0
      stencil.SetZero();
      *stencil.Pointer(0, 0) = 1;
      *stencil.Pointer(0, spacing-1) = -2;
      *stencil.Pointer(0, stencil.get_size(1)-1) = 1;

      //dg2 = conv(stencil, gaussian_kernel(sigma));
      FixedPointArray<s32> gaussian = Get1dGaussianKernel(sigma, numSigmaFractionalBits, numStandardDeviations, scratch); // SQ23.8
      FixedPointArray<s32> differenceOfGaussian(1, gaussian.get_size(1)+stencil.get_size(1)-1, numSigmaFractionalBits, scratch); // SQ23.8

      if(Correlate1d(stencil, gaussian, differenceOfGaussian) != RESULT_OK)
        return RESULT_FAIL;

      FixedPointArray<s32> boundaryXFiltered(1, boundary.get_size(), numSigmaFractionalBits, scratch); // SQ23.8
      FixedPointArray<s32> boundaryYFiltered(1, boundary.get_size(), numSigmaFractionalBits, scratch); // SQ23.8

      //differenceOfGaussian.Print("differenceOfGaussian");

      //r_smooth = imfilter(boundary, dg2(:), 'circular');
      {
        PUSH_MEMORY_STACK(scratch);
        FixedPointArray<s32> boundaryX(1, boundary.get_size(), 0, scratch); // SQ31.0

        const Point<s16> * restrict boundary_constRowPointer = boundary.Pointer(0);
        s32 * restrict boundaryX_rowPointer = boundaryX.Pointer(0,0);

        for(s32 i=0; i<boundary.get_size(); i++) {
          boundaryX_rowPointer[i] = boundary_constRowPointer[i].x;
        }

        if(Correlate1dCircularAndSameSizeOutput(boundaryX, differenceOfGaussian, boundaryXFiltered) != RESULT_OK)
          return RESULT_FAIL;

        //boundaryX.Print("boundaryX");
        //boundaryXFiltered.Print("boundaryXFiltered");
      } // PUSH_MEMORY_STACK(scratch);

      {
        PUSH_MEMORY_STACK(scratch);
        FixedPointArray<s32> boundaryY(1, boundary.get_size(), 0, scratch); // SQ31.0

        const Point<s16> * restrict boundary_constRowPointer = boundary.Pointer(0);
        s32 * restrict boundaryY_rowPointer = boundaryY.Pointer(0,0);

        for(s32 i=0; i<boundary.get_size(); i++) {
          boundaryY_rowPointer[i] = boundary_constRowPointer[i].y;
        }

        if(Correlate1dCircularAndSameSizeOutput(boundaryY, differenceOfGaussian, boundaryYFiltered) != RESULT_OK)
          return RESULT_FAIL;

        //boundaryY.Print("boundaryY");
        //boundaryYFiltered.Print("boundaryYFiltered");
      } // PUSH_MEMORY_STACK(scratch);

      //r_smooth = sum(r_smooth.^2, 2);
      FixedPointArray<s32> boundaryFilteredAndCombined(1, boundary.get_size(), 2*numSigmaFractionalBits, scratch); // SQ15.16
      s32 * restrict boundaryFilteredAndCombined_rowPointer = boundaryFilteredAndCombined.Pointer(0,0);

      const s32 * restrict boundaryXFiltered_constRowPointer = boundaryXFiltered.Pointer(0,0);
      const s32 * restrict boundaryYFiltered_constRowPointer = boundaryYFiltered.Pointer(0,0);

      for(s32 i=0; i<boundary.get_size(); i++) {
        //const s32 xSquared = (boundaryXFiltered_constRowPointer[i] * boundaryXFiltered_constRowPointer[i]) >> numSigmaFractionalBits; // SQ23.8
        //const s32 ySquared = (boundaryYFiltered_constRowPointer[i] * boundaryYFiltered_constRowPointer[i]) >> numSigmaFractionalBits; // SQ23.8
        const s32 xSquared = (boundaryXFiltered_constRowPointer[i] * boundaryXFiltered_constRowPointer[i]); // SQ31.0 (multiplied by 2^numSigmaFractionalBits)
        const s32 ySquared = (boundaryYFiltered_constRowPointer[i] * boundaryYFiltered_constRowPointer[i]); // SQ31.0 (multiplied by 2^numSigmaFractionalBits)

        boundaryFilteredAndCombined_rowPointer[i] = xSquared + ySquared;
      }

      FixedLengthList<s32> localMaxima(maximumTemporaryPeaks, scratch);

      const s32 * restrict boundaryFilteredAndCombined_constRowPointer = boundaryFilteredAndCombined.Pointer(0,0);

      // Find local maxima -- these should correspond to the corners of the square.
      // NOTE: one of the comparisons is >= while the other is >, in order to
      // combat rare cases where we have two responses next to each other that are exactly equal.
      // localMaxima = find(r_smooth >= r_smooth([end 1:end-1]) & r_smooth > r_smooth([2:end 1]));

      if(boundaryFilteredAndCombined_constRowPointer[0] > boundaryFilteredAndCombined_constRowPointer[1] &&
        boundaryFilteredAndCombined_constRowPointer[0] >= boundaryFilteredAndCombined_constRowPointer[boundary.get_size()-1]) {
          localMaxima.PushBack(0);
      }

      for(s32 i=1; i<(boundary.get_size()-1); i++) {
        if(boundaryFilteredAndCombined_constRowPointer[i] > boundaryFilteredAndCombined_constRowPointer[i+1] &&
          boundaryFilteredAndCombined_constRowPointer[i] >= boundaryFilteredAndCombined_constRowPointer[i-1]) {
            localMaxima.PushBack(i);
        }
      }

      if(boundaryFilteredAndCombined_constRowPointer[boundary.get_size()-1] > boundaryFilteredAndCombined_constRowPointer[0] &&
        boundaryFilteredAndCombined_constRowPointer[boundary.get_size()-1] >= boundaryFilteredAndCombined_constRowPointer[boundary.get_size()-2]) {
          localMaxima.PushBack(boundary.get_size()-1);
      }

      //boundaryFilteredAndCombined.Print("boundaryFilteredAndCombined");
      //for(s32 i=0; i<boundaryFilteredAndCombined.get_size(1); i++) {
      //  printf("%d\n", boundaryFilteredAndCombined[0][i]);
      //}

      //localMaxima.Print("localMaxima");

      const Point<s16> * restrict boundary_constRowPointer = boundary.Pointer(0);

      //localMaxima.Print("localMaxima");

      // Select the index of the top 4 local maxima
      // TODO: make efficient
      // TODO: make work for numbers of peaks other than 4
      s32 * restrict localMaxima_rowPointer = localMaxima.Pointer(0);
      s32 maximaValues[4] = {s32_MIN,s32_MIN,s32_MIN,s32_MIN};
      s32 maximaIndexes[4] ={-1,-1,-1,-1};
      for(s32 iMax=0; iMax<4; iMax++) {
        for(s32 i=0; i<localMaxima.get_size(); i++) {
          const s32 localMaximaIndex = localMaxima_rowPointer[i];
          if(boundaryFilteredAndCombined_constRowPointer[localMaximaIndex] > maximaValues[iMax]) {
            maximaValues[iMax] = boundaryFilteredAndCombined_constRowPointer[localMaximaIndex];
            maximaIndexes[iMax] = localMaximaIndex;
          }
        }
        //printf("Maxima %d/%d is #%d %d\n", iMax, 4, maximaIndexes[iMax], maximaValues[iMax]);
        boundaryFilteredAndCombined_rowPointer[maximaIndexes[iMax]] = s32_MIN;
      }

      // Copy the maxima to the output peaks, ordered by their original index order, so they are
      // still in clockwise or counterclockwise order
      peaks.Clear();

      bool whichUsed[4] = {false,false,false,false};
      for(s32 iMax=0; iMax<4; iMax++) {
        s32 curMinIndex = -1;
        for(s32 i=0; i<4; i++) {
          if(!whichUsed[i] && maximaIndexes[i] >= 0) {
            if(curMinIndex == -1 || maximaIndexes[curMinIndex] > maximaIndexes[i]) {
              curMinIndex = i;
            }
          }
        }

        //if(maximaIndexes[iMax] >= 0) {
        if(curMinIndex >= 0) {
          whichUsed[curMinIndex] = true;
          peaks.PushBack(boundary_constRowPointer[maximaIndexes[curMinIndex]]);
        }
      }

      //boundary.Print();
      //peaks.Print();

      return RESULT_OK;
    } // Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki