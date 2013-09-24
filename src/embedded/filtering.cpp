#include "anki/embeddedVision/miscVisionKernels.h"

using namespace std;

namespace Anki
{
  namespace Embedded
  {
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

      // TODO: allow different numbers of fractional bits
      /*AnkiConditionalErrorAndReturnValue(in1.get_numFractionalBits()==in2.get_numFractionalBits() && in1.get_numFractionalBits()==out.get_numFractionalBits(),
      RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "All arrays must have the same number of fractional bits");*/

      // TODO: allow this
      /*AnkiConditionalErrorAndReturnValue((in1.get_numFractionalBits()+in2.get_numFractionalBits()) > out.get_numFractionalBits(),
      RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "Out has too many fractional bits");*/

      AnkiConditionalErrorAndReturnValue(out.get_size(1) == outputLength,
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "Out must be the size of in1 + in2 - 1");

      AnkiConditionalErrorAndReturnValue(in1.get_rawDataPointer() != in2.get_rawDataPointer() && in1.get_rawDataPointer() != out.get_rawDataPointer(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "in1, in2, and out must have be in different memory locations");

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
      const s32 midEndIndex = midStartIndex + uLength - vLength ;

      typedef enum
      {
        BITSHIFT_NONE,
        BITSHIFT_LEFT,
        BITSHIFT_RIGHT
      } BitShiftDirection;

      s32 bitShift;
      BitShiftDirection direction;
      if((in1.get_numFractionalBits()+in2.get_numFractionalBits()) > out.get_numFractionalBits()) {
        direction = BITSHIFT_RIGHT;
        bitShift = in1.get_numFractionalBits() + in2.get_numFractionalBits() - out.get_numFractionalBits();
      } else if((in1.get_numFractionalBits()+in2.get_numFractionalBits()) < out.get_numFractionalBits()) {
        direction = BITSHIFT_LEFT;
        bitShift = out.get_numFractionalBits() - in1.get_numFractionalBits() - in2.get_numFractionalBits();
      } else {
        direction = BITSHIFT_NONE;
      }

      s32 iOut = 0;

      // Filter the left part
      //printf("left: ");
      for(s32 x=0; x<midStartIndex; x++) {
        //printf("%d ", x);
        s32 sum = 0;
        for(s32 xx=0; xx<=x; xx++) {
          const s32 toAdd = (u_rowPointer[xx] * v_rowPointer[vLength-x+xx-1]);
          sum += toAdd;
        }

        // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
        if(direction == BITSHIFT_RIGHT) {
          out_rowPointer[iOut++] = sum >> bitShift;
        } else if(direction == BITSHIFT_LEFT) {
          out_rowPointer[iOut++] = sum << bitShift;
        } else {
          out_rowPointer[iOut++] = sum;
        }
      }

      // Filter the middle part
      //printf("mid: ");
      for(s32 x=midStartIndex; x<=midEndIndex; x++) {
        //printf("%d ", x);
        s32 sum = 0;
        for(s32 xx=0; xx<vLength; xx++) {
          const s32 toAdd = (u_rowPointer[x+xx-midStartIndex] * v_rowPointer[xx]);
          sum += toAdd;
        }

        // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
        if(direction == BITSHIFT_RIGHT) {
          out_rowPointer[iOut++] = sum >> bitShift;
        } else if(direction == BITSHIFT_LEFT) {
          out_rowPointer[iOut++] = sum << bitShift;
        } else {
          out_rowPointer[iOut++] = sum;
        }
      }

      // Filter the right part
      //printf("right: ");
      for(s32 x=midEndIndex+1; x<outputLength; x++) {
        //printf("%d ", x);
        const s32 vEnd = outputLength - x;
        s32 sum = 0;
        for(s32 xx=0; xx<vEnd; xx++) {
          const s32 toAdd = (u_rowPointer[x+xx-midStartIndex] * v_rowPointer[xx]);
          sum += toAdd;
        }

        // TODO: if this is too slow and the compiler doesn't figure it out, pull these ifs out
        if(direction == BITSHIFT_RIGHT) {
          out_rowPointer[iOut++] = sum >> bitShift;
        } else if(direction == BITSHIFT_LEFT) {
          out_rowPointer[iOut++] = sum << bitShift;
        } else {
          out_rowPointer[iOut++] = sum;
        }
      }
      //printf("\n\n");

      return RESULT_OK;
    }

    Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16>> &boundary, MemoryStack scratch)
    {
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
      FixedPointArray<s32> differenceOfGaussian(1, gaussian.get_size(1), numSigmaFractionalBits, scratch); // SQ23.8

      if(Correlate1d(stencil, gaussian, differenceOfGaussian) != RESULT_OK)
        return RESULT_FAIL;

      //r_smooth = imfilter(boundary, dg2(:), 'circular');
      //r_smooth = sum(r_smooth.^2, 2);

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki