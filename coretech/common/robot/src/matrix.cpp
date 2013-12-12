/**
File: matrix.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/matrix.h"

#include "anki/common/robot/opencvLight.h"
#include "anki/common/robot/matlabInterface.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Matrix
    {
      static Result lastResult;

      Result SolveLeastSquaresWithSVD_f32(Array<f32> &At, const Array<f32> &bt, Array<f32> &xt, MemoryStack scratch)
      {
        const s32 AWidth = At.get_size(0);

        Array<f32> APrime;
        Array<f32> bPrime;
        Array<f32> w(1, AWidth, scratch);
        Array<f32> U(AWidth, AWidth, scratch);
        Array<f32> V(AWidth, AWidth, scratch);

        if(At.get_size(0) == At.get_size(1)) {
          APrime = At;
          bPrime = bt;
        } else {
          APrime = Array<f32>(AWidth, AWidth, scratch);
          bPrime = Array<f32>(AWidth, 1, scratch);

          // A' * A = A'A
          if((lastResult = MultiplyTranspose<f32,f32>(At, At, APrime)) != RESULT_OK)
            return lastResult;

          // A' * b = A'b
          if((lastResult = MultiplyTranspose<f32,f32>(At, bt, bPrime)) != RESULT_OK)
            return lastResult;
        }

        const s32 AMinStride = RoundUp<s32>(AWidth, MEMORY_ALIGNMENT);

        {
          PUSH_MEMORY_STACK(scratch);
          void * svdScratch = scratch.Allocate(sizeof(f32)*(AMinStride*3) + 64);
          if((lastResult = svd_f32(APrime, w, U, V, svdScratch))  != RESULT_OK)
            return lastResult;
        } // PUSH_MEMORY_STACK(scratch);

        {
          PUSH_MEMORY_STACK(scratch);
          void * svdScratch = scratch.Allocate(sizeof(f32)*(AMinStride*2) + 64);

          Array<f32> bPrimeTransposed(1, AWidth, scratch);
          Reshape(true, bPrime, bPrimeTransposed);

          if((lastResult = svdBackSubstitute_f32(w, V, V, bPrimeTransposed, xt, svdScratch))  != RESULT_OK)
            return lastResult;
        } // PUSH_MEMORY_STACK(scratch);

        return RESULT_OK;
      }

      Result SolveLeastSquaresWithSVD_f64(Array<f64> &At, const Array<f64> &bt, Array<f64> &xt, MemoryStack scratch)
      {
        const s32 AWidth = At.get_size(0);

        Array<f64> APrime;
        Array<f64> bPrime;
        Array<f64> w(1, AWidth, scratch);
        Array<f64> U(AWidth, AWidth, scratch);
        Array<f64> V(AWidth, AWidth, scratch);

        if(At.get_size(0) == At.get_size(1)) {
          APrime = At;
          bPrime = bt;
        } else {
          APrime = Array<f64>(AWidth, AWidth, scratch);
          bPrime = Array<f64>(AWidth, 1, scratch);

          // A' * A = A'A
          if((lastResult = MultiplyTranspose<f64,f64>(At, At, APrime)) != RESULT_OK)
            return lastResult;

          // A' * b = A'b
          if((lastResult = MultiplyTranspose<f64,f64>(At, bt, bPrime)) != RESULT_OK)
            return lastResult;
        }

        const s32 AMinStride = RoundUp<s32>(AWidth, MEMORY_ALIGNMENT);

        {
          PUSH_MEMORY_STACK(scratch);
          void * svdScratch = scratch.Allocate(sizeof(f64)*(AMinStride*3) + 64);
          if((lastResult = svd_f64(APrime, w, U, V, svdScratch))  != RESULT_OK)
            return lastResult;
        } // PUSH_MEMORY_STACK(scratch);

        {
          PUSH_MEMORY_STACK(scratch);
          void * svdScratch = scratch.Allocate(sizeof(f64)*(AMinStride*2) + 64);

          Array<f64> bPrimeTransposed(1, AWidth, scratch);
          Reshape(true, bPrime, bPrimeTransposed);

          if((lastResult = svdBackSubstitute_f64(w, V, V, bPrimeTransposed, xt, svdScratch))  != RESULT_OK)
            return lastResult;
        } // PUSH_MEMORY_STACK(scratch);

        return RESULT_OK;
      }
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki